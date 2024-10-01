/* compile.c -- Compile scheme expressions into byte-codes.
	written by Jason Coughlin

   Version 1

	- Byte-code is generated in a codebuffer since the size of the
	code and the constant table isn't known until after compilation.
	The code and constant table are then copied into a byte-code CONS
	node.

	- Byte-code is a CONS node with code and a constant table.  Since
	it's a CONS node, old byte-code will be garbage collected.

	- Since the codebuffers are local to compile.c, the constants put
	into a codebuffer's constant table have to be protected from a gc.
	Most constants are already protected because mcCompile() copies
	the expression and saves it in a register.  However, cpLambda()
	generates a constant.  This constant has to be pushed on the
	register stack to save it from a GC.  It's popped off when
	mcCompile() calls MCLEAVE.

	- MAX_BCONST is 256 so that pointers into the constant table are
	only 1 byte.

   BUGS:
	- I can't figure out why this is a problem, but a GC CAN'T
	happen during cpCompileArgs().  cpCompileArgs() reverses
	the arg list and in so doing, orphans the args.  I put the
	orphaned args in a register, but they still disappeared.  It's
	related to the destructive reverse, but ...
*/

#include "machine.h"

#ifdef AllocH
#	include <alloc.h>
#endif

#include MEMORY_H
#include STDLIB_H

#include "glo.h"
#include "symstr.h"
#include "memory.h"
#include "micro.h"
#include "eval.h"
#include "predefs.h"
#include "compile.h"
#include "debug.h"
#include "error.h"

int cp_debug;

/* maximums for the code-buffers */
#define MAX_BCODE	512
#define MAX_CONST	256

/* this structure must be public so the marking algorithm
 * will mark the constant table.
 */
typedef struct {
	unsigned char code[MAX_BCODE];		/* the generated code */
	CONS cnst[MAX_CONST];			/* the constants */
	unsigned int codeptr;			/* code pntr */
	unsigned int cnstptr;			/* constant table pntr */
} CODENODE;
typedef CODENODE *CODE_BUFFER;

/* macros to generate code in a codebuffer */
#define cpCode(n,i)	( (n)->code[((n)->codeptr)++] = (i) )
#define cpGetCP(n)	( (n)->cnstptr )
#define cpGetIP(n)	( (n)->codeptr )
#define cpSetCP(n,k)	( (n)->cnstptr = (k) )
#define cpSetIP(n,i)	( (n)->codeptr = (i) )
#define cpFixup(n,a,i)	( (n)->code[(a)] = (unsigned char) (i) )

/* compiled code buffer -- where the generated code is placed while
 * expressions are being compiled.
 */
static CODE_BUFFER glo_cbuffer;

/* local prototypes */
static CONS cpMakeBCode( C_CODE_BUFFER );
static void cpDumpBC( C_CONS );
static void cpConst( C_CODE_BUFFER X C_CONS );
static void cpCompile( C_CODE_BUFFER X C_CONS X C_INT );
static void cpCompilePrim( C_CODE_BUFFER X C_CONS X C_CONS );
static void cpCompileForm( C_CODE_BUFFER X C_CONS X C_CONS X C_INT );
static void cpCompileArgs( C_CODE_BUFFER X C_CONS );
static void cpIf( C_CODE_BUFFER X C_CONS X C_INT );
static void cpBegin( C_CODE_BUFFER X C_CONS X C_INT );
static void cpQuote( C_CODE_BUFFER X C_CONS X C_INT );
static void cpLambda( C_CODE_BUFFER X C_CONS X C_INT );
static void cpDefine( C_CODE_BUFFER X C_CONS X C_INT );
static void cpSet( C_CODE_BUFFER X C_CONS X C_INT );

/* InitComp() - Initialize the compiler. */
void InitComp(argc, argv)
int argc;
char *argv[];
{
   int i;

   cp_debug = FALSE;

   /* allocate the main compile buffer */
   if ( (glo_cbuffer = (CODE_BUFFER) malloc( sizeof(CODENODE) )) == NULL ) {
	fprintf(stdout, "\nFATAL ERROR IN init_comp(): Out of memory.\n");
	exit(1);
   }

   cpSetIP(glo_cbuffer, 0);
   cpSetCP(glo_cbuffer, 0);

   for ( i = 1; i < argc; ++i ) {
	if ( argv[i][0] == '-' || argv[i][0] == '/' ) {
		switch ( argv[i][1] ) {
		   case 'c':
			cp_debug = TRUE;
			break;
		}
	}
   }
}

/* ----------------------------------------------------------------------- */
/*                               Compiler				   */
/* ----------------------------------------------------------------------- */

/* mcCompile(e) - Fully compile expression e. */
CONS mcCompile(e)
CONS e;
{
   ENTER;
   REG(bcode);
   REG(exp);

   /* copy the expression to avoid capture */
   R(exp) = mcTree_Copy(e);

   /* initialize the code buffer */
   cpSetIP(glo_cbuffer, 0);
   cpSetCP(glo_cbuffer, 0);

   /* compile the expression */
   cpCompile(glo_cbuffer, e, TRUE);

   /* copy into a byte-code node */
   R(bcode) = cpMakeBCode( glo_cbuffer );

   if ( cp_debug )
	cpDumpBC( R(bcode) );

   MCLEAVE R(bcode);
}

/* cpMakeBCode(cb) -- Copy the byte-code from the code-buffer cb into a
	cons node.
*/
static CONS cpMakeBCode(cb)
CODE_BUFFER cb;
{
   int l;
   CONS code;

   /* copy the code into a BCODE node */
   code = NewCons( BCODES, cpGetIP(cb), cpGetCP(cb) );

   /* copy the code into this BCODE node */
   memcpy( (char *)mcBC_Code(code), (char *)cb->code, (int)cpGetIP(cb) );

   /* copy the constants */
   for ( l = 0; l < cpGetCP(cb); ++l )
	*(mcBC_Const(code)+l) = cb->cnst[l];

   return code;
}

/* cpConst(cb, k) - Add e to the constant table in code-buffer cb. */
static void cpConst(cb, k)
CODE_BUFFER cb;
CONS k;
{
   cb->cnst[ cpGetCP(cb) ] = k;
   ++cb->cnstptr;
}

/* cpCompile(cb, e, at_end) - Compile expression e into code-buffer cb.  To
	handle tail-recursion properly, at_end is TRUE if we're in a
	sequence (begin) -- FALSE otherwise.
*/
static void cpCompile(cb, e, at_end)
CODE_BUFFER cb;
CONS e;
int at_end;
{
   CONS f, args, binding;

   CP_DEBUG("\nIn cpCompile, e = ", e);

   /* compiling an atom */
   if ( !mcPair(e) ) {

	if ( mcNumber(e) || mcString(e) || mcNull(e) || mcChar(e) ) {
		/* code generated: prPushConst cnstptr
		 * constant table: add e
		 */
		cpCode( cb, prPushConst );
		cpCode( cb, cpGetCP(cb) );
		cpConst( cb, e );
		return;
	}

	if ( mcSymbol(e) ) {
		/* code generated: prPushVar cnstptr
		 * constant table: add e
		 */
		cpCode( cb, prPushVar );
		cpCode( cb, cpGetCP(cb) );
		cpConst( cb, e );
		return;
	}
   }

   /* e is a list of the form: (f arg1 ...) */
   f = mcCar(e);
   args = mcCdr(e);

   /* if f is an atom, it might be a system-defined function */
   if ( !mcPair(f) ) {
	/* system funcs are in the environment */
	binding = evAccGlobal(f, glo_env);
	if ( binding != NULL ) {

		/* compile a system special form */
		if ( mcForm(binding) ) {
			cpCompileForm( cb, binding, args, at_end );
			return;
		}

		/* compile a primitive function */
		if ( mcFunc(binding) ) {
			cpCompilePrim( cb,binding, args );
			return;
		}
	}
   }

   /* e is an application of a user-defined closure or form
    * so push a mark so the interpreter knows what to bind as args.
    */
   cpCode( cb, prPushMark );

   /* compile f
    *   -- if f is (lambda (h k) (+ h k)) then cpCompile(f) will use
    *	   cpLambda() to generate a "Make closure" instruction.
    *   -- if f is an atom, then cpCompile(f) will generate a lookup
    *      instruction
    */
   cpCompile(cb, f, at_end);

   /* move the closure from the val stack to the function stack */
   cpCode( cb, prPushFunc );

   /* generate code to "evaluate the arguments" */
   cpCompileArgs( cb, args );

   /* call the user function */
   cpCode( cb, prCall );
}

/* cpCompileArgs(a) -- Compile the arguments to a function. */
static void cpCompileArgs(cb, args)
CODE_BUFFER cb;
CONS args;
{
   CONS sargs, farg;

   /* reverse arg-list so args are in the right order */
   sargs = args = mcRev(args);

   while ( !mcNull(args) ) {
	/* strip off first arg (actually the last :-) */
	farg = mcCar(args);
	args = mcCdr(args);

	/* compile its arguments; we are definitely NOT at the end of an
   	 * evaluation ready to return a value.
   	 */
	cpCompile(cb, farg, FALSE);
   }

   /* restore args to previous state */
   sargs = mcRev( sargs );
}

/* cpCompilePrim(func) -- Generate code for a primtive function.  func is
	the primitive's CFUNC definition.
*/
static void cpCompilePrim(cb, func, args)
CODE_BUFFER cb;
CONS func, args;
{
   int nargs;

   CP_DEBUG("\nCompiling primitive function.", NIL);

   /* check the # of args */
   nargs = mcLength(args);
   if ( !((mcPrim_RA(func) == nargs ) ||
         (mcPrim_AA(func) >= mcPrim_RA(func) && nargs == mcPrim_AA(func)) ||
         (mcPrim_AA(func) < mcPrim_RA(func)  && nargs >= mcPrim_RA(func))) ) {
	/* wrong # of args */
	fprintf(currout, "Error: COMPILE: Wrong # of args to primitive procedure %s: ", mcPrim_Name(func));
	mcWrite(args, currout);
	fprintf(currout, "\n\n");
	ERROR;
   }

   /* functions with variable # of args need the val stack marked. */
   if ( mcPrim_RA(func) != mcPrim_AA(func) )
	cpCode( cb, prPushMark );

   /* compile the arguments: the arguments aren't in a sequence */
   cpCompileArgs( cb, args );

   /* execute the primitive */
   cpCode( cb, mcPrim_PR(func) );
}

/* cpCompileForm(f, e) -- Compile a system form. */
static void cpCompileForm(cb, f, e, at_end)
CODE_BUFFER cb;
CONS f, e;
int at_end;
{
   switch ( mcPrim_PR(f) ) {
	case prBegin:
		cpBegin(cb, e, at_end);
		break;
	case prIf:
		cpIf(cb, e, at_end);
		break;
	case prQuote:
		cpQuote(cb, e, at_end);
		break;
	case prLambda:
		cpLambda(cb, e, at_end);
		break;
	case prDefine:
		cpDefine(cb, e, at_end);
		break;
	case prSet:
		cpSet(cb, e, at_end);
		break;
   }
}

/* cpIf(e) -- Compile an 'if' expression.  e looks like this:
	(conditional-expr then-expr else-expr)
*/
static void cpIf(cb, e, at_end)
CODE_BUFFER cb;
CONS e;
int at_end;
{
   char goto_else, goto_done;

   CP_DEBUG("\nCompiling if.", NIL);

   /* compile conditional-expr */
   cpCompile( cb, mcCar(e), FALSE );

   /* generate prNilBranch -- save the address of the NilBranch so we
    * can fill it in later.
    */
   cpCode( cb, prNilBranch );
   goto_else = cpGetIP(cb);

   /* reserve space for the address to branch to */
   cpCode( cb, prNoOp );

   /* compile then expr */
   cpCompile( cb, mcCadr(e), at_end );

   /* if we're at the end (ie, ready to return a value) generate a return
    * instruction so we can take of tail-recursion.  if we're not at the
    * end, generate a branch to the end of the if statement.
    */
   if ( at_end )
	cpCode( cb, prReturn );
   else {
	/* generate prBranch -- save the address of the branch so we
	 * can fill it in later.
	 */
	cpCode( cb, prBranch );

	goto_done = cpGetIP(cb);
	cpCode( cb, prNoOp );
   }

   /* compile else-expr: fixup the NilBranch since we now know where it's
    * going.
    * THIS WOULD CHANGE IF THE COMPILER IS MODIFIED TO GENERATE MORE THAN
    * 256 BYTES OF CODE.
    */
   cpFixup( cb, goto_else, cpGetIP(cb) );
   cpCompile( cb, mcCaddr(e), at_end );

   /* fixup so that the then-expr branches to this point
    * THIS WOULD CHANGE IF THE COMPILER IS MODIFIED TO GENERATE MORE THAN
    * 256 BYTES OF CODE.
    */
   if ( at_end ) {
	cpCode( cb, prReturn );
   } else {

	cpFixup( cb, goto_done, cpGetIP(cb) );

	/* compile a NoOp that the then-expr will branch to */
	cpCode( cb, prNoOp );
   }
}

/* cpBegin(e) -- Compile the 'begin' special form.  e looks like:
	(exp1 ...)
*/
static void cpBegin(cb, e, at_end)
CODE_BUFFER cb;
CONS e;
int at_end;
{
   CONS exp;

   CP_DEBUG("\nCompiling Begin.", NIL);
   while ( !mcNull(e) ) {

	exp = mcCar(e);
	e = mcCdr(e);

	if ( !mcNull(e) ) {
		/* compile first exp; we're in a sequence now so we're
		 * not at the end of evaluation ready to return a value
		 */
		cpCompile( cb, exp, FALSE );

		/* generate brPopVal to pop off the last evaluation */
		cpCode( cb, prPopVal );
	} else {
		/* value of begin is the value of the last expression.
		 * NOTE: we're ready to return a value only if our
		 * caller is ready to return a value.
		 */
		cpCompile( cb, exp, at_end );
	}
   }
}

/* cpQuote(e) -- Compile (quote exp) */
static void cpQuote(cb, e, at_end)
CODE_BUFFER cb;
CONS e;
int at_end;
{
   CP_DEBUG("\nCompiling quote.", NIL);
   cpCode( cb, prPushConst );
   cpCode( cb, cpGetCP(cb) );
   cpConst( cb, mcCar(e) );
}

/* cpLambda(e) -- Compile a lambda expression */
static void cpLambda(cb, e, at_end)
CODE_BUFFER cb;
CONS e;
int at_end;
{
   /* this routine can't use the ENTER macro!  it pushes the new closure
    * on the register stack to protect it from a garbage-collection.  if
    * ENTER were used, the closure would be popped off the reg stack
    * when the corresponding MCLEAVE were issued.
    */
   CONS lcode;
   CODE_BUFFER lcb;

   CP_DEBUG("\nCompiling lambda.", NIL);

   /* allocate a new code-buffer to hold code for the body of the lambda */
   if ( (lcb = (CODE_BUFFER) malloc( sizeof(CODENODE) )) == NULL ) {
	RT_ERROR("Out of memory for compilation");
   }
   cpSetIP(lcb, 0);
   cpSetCP(lcb, 0);

   /* compile the body of the lambda expression */
   cpBegin( lcb, mcCdr(e), at_end );

   /* move the compiled body into it's own byte-code node and push the
    * byte-code on the register stack so it won't disappear with a
    * garbage collection.
    */
   lcode = cpMakeBCode(lcb);
   mcRegPush( lcode );

   if ( cp_debug )
	cpDumpBC(lcode);

   /* generate a "Make closure" instruction in the current code-buffer:
    *
    * push the parm-list
    */
   cpCode( cb, prPushConst );
   cpCode( cb, cpGetCP(cb) );
   cpConst( cb, mcCar(e) );

   /* push the lambda's byte-code */
   cpCode( cb, prPushConst );
   cpCode( cb, cpGetCP(cb) );
   cpConst( cb, lcode );

   /* make a closure */
   cpCode( cb, prMakeClosure );
}

/* cpDefine(e) - Compile 'define'. */
static void cpDefine(cb, e, at_end)
CODE_BUFFER cb;
CONS e;
int at_end;
{
   if ( !mcSymbol( mcCar(e) ) ) {
	RT_LERROR("COMPILE: Illegal DEFINE syntax.  Can't bind to non-symbol: ", mcCar(e) );
   }

   /* push the symbol to be defined */
   cpCode( cb, prPushConst );
   cpCode( cb, cpGetCP(cb) );
   cpConst( cb, mcCar(e) );

   /* compile the expression */
   cpCompile( cb, mcCadr(e), FALSE );

   cpCode( cb, prDefine );
}

/* cpSet(e) - Compile 'set!' */
static void cpSet(cb, e, at_end)
CODE_BUFFER cb;
CONS e;
int at_end;
{
   if ( !mcSymbol( mcCar(e) ) ) {
	RT_LERROR("COMPILE: Illegal SET! syntax.  Can't bind to non-symbol: ", mcCar(e) );
   }

   /* push the symbol to be defined */
   cpCode( cb, prPushConst );
   cpCode( cb, cpGetCP(cb) );
   cpConst( cb, mcCar(e) );

   /* compile the expression */
   cpCompile( cb, mcCadr(e), FALSE );

   cpCode( cb, prSet );
}

/* cpDumpBC(n) -- Given a byte-code node, dumps it's contents. */
static void cpDumpBC(n)
CONS n;
{
   unsigned int l;

   assert( mcCode(n) );

   printf("\n\nGenerated code: size: %d, # of consts: %d\n",
	mcBC_CSize(n), mcBC_CCSize(n) );

   for ( l = 0; l < mcBC_CSize(n); ++l )
	printf("%d ", *(mcBC_Code(n)+l) );

   printf("\n\nConstants:\n");
   for ( l = 0; l < mcBC_CCSize(n); ++l ) {
	printf("%u = ", l);
	mcWrite( *(mcBC_Const(n)+l), currout );
	printf("\n");
   }
}
