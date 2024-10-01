/* eval.c - Scheme Evaluation Routine
	written by Jason Coughlin (jk0@sun.soe.clarkson.edu)

   Version 3 (jk0) New environments

	- Environments are broken into 2 pieces: a nested env which holds
	parameters and local bindings (nested scopes), and a global
	environment which holds global bindings (created with the define
	special form).

	- The nested env is an A-LIST and is maintained by CONSing bindings
	on the front and using RESTORE to restore previous nestings.  For
	the interpreter, lookup	of variables in the nested env is
	O( |nested_env| ).  For the compiler, lookup of variables in the
	nested env can be optimized to O( k ) also (because of static scope).

	- The global env's bindings are actually stored in a vector indexed
	by the symbol.  A symbol is no longer a pointer to the string but
	the index in the symbol table.  For both the interpreter and the
	compiler, lookup of bindings in the global env is O( k ).

	- Net effect on lookup of variables:
		interpreter: O( |nested_env| ).
		compiler   : O( |nested_env| ).  [ O( k ) optimized ]

	- Only the nested portion of an environment is pushed on the
	stack for restoration, saved in a continuation, and saved in
	a closure.  That's all that's needed.

   Version 2 (jk0) Changed to Stack-based machine

	- An execution point is a byte-code RESUME -- it saves the state of
	interpreting byte-code.  An exe-point is a pointer to byte-code,
	a pc, and the environment in effect for that code.  The pc is the
	NEXT instruction to execute.

	- Tail recursion is handled by eval.  It is eliminated by not pushing
	an execution point if the next instruction to be executed is either
	out of the byte-code or is a RETURN instruction.  In this way, the
	invokation returns immediately to the byte-code's caller.

	- Evaluation of special forms which need to evaluate some of their
	args is "sticky".
		(1) The operation for the form is called.  This operation
		sets up a RESUME and pushes what it needs evaluated on the
		expression stack.  The RESUME is a special CONS node with
		the PR # of the form and a list of anything the form needs
		to complete it's work.

		(2) The operation returns to EVAL which sees stuff to
		evaluate and evaluates it.

		(3) EVAL pops the RESUME from the expression stack, and
		invokes the form's "resume" function.

	- Evaluation of a continuation:
		(1) Restore the expression stack, the value stack, and
		the environment.

		(2) Looks for something on the expression stack to evaluate.

	- Evaluation of (LAMBDA (parms) (body)) returns a CLOSURE.  A
	closure is code, the parameters, and the environment at the time
	the closure is defined.

	- (DEFINE (name arg1 ...) body) is a special form of
	(DEFINE name (LAMBDA (arg1 ...) body)).

	- Evaluation of a CLOSURE:
		(1) Evaluate the arguments to a closure.
		(2) Save the current environment.
		(3) Bind the arguments to the parameters on top of the
		environment IN THE CLOSURE (static scope).
		(4) Evaluate the body with the new environment (push the
		body on the expression stack.)
		(5) Return the result on the value-stack.

	- Handling of tail recursion.  At step (2) above, the interpreter
	looks to see if an environment is already on the expression stack.
	If it is, then there is no need to push another!  When the closure
	is done, we'll just pop back to the original env.  Otherwise, we'd
	have to backtrack through ALL of the saved envs.

   Version 1 (jk0) - Recursive model

	- No tail-recursion handling.

	- Environment is an A-LIST maintained by CONSing bindings on the
	front and using RESTORE to restore previous environments.

   BUGS:

*/

#include "machine.h"

#include STRING_H

#include "glo.h"
#include "symstr.h"
#include "memory.h"
#include "micro.h"
#include "error.h"
#include "eval.h"
#include "ops.h"
#include "preds.h"
#include "forms.h"
#include "predefs.h"

/* global variables */
int eval_debug;

/* the function lookup table for the byte-code interpreter */
static void (*BOPS[NUM_FUNCS])( C_VOID );

/* local support routines */
static CONS evEvalAtom( C_CONS );
static void evCountArgs( C_CONS );
static CONS evBindArgs( C_CONS X C_CONS );
static CONS evBindFormArgs( C_CONS X C_CONS );
static void evInvokeUserFunc( C_CONS X C_CONS X C_CONS );
static void evInvokeUserForm( C_CONS X C_CONS );
static void evInvokeCont( C_CONS );
static void evInvokeForm( C_CONS );
static void evInvokeRes( C_CONS );

static void evInvokeBC( C_CONS );
static void evSaveExe( C_INT X C_CONS );

static int evExpandOnce( C_CONS );
static void evResExpand( C_VOID );

static void mcDumpStacks( C_VOID );
static void mcDumpStack( C_CONS C_PTR X C_CONS C_ARRAY );

/* InitEval() - Initialize the interpreters. */
void InitEval(argc, argv)
int argc;
char *argv[];
{
   int i;

   eval_debug = FALSE;

   /* initialize the byte-code function table */
   for ( i = 0; i < NUM_FUNCS; i++ )
	BOPS[i] = opNoOp;

   for ( i = 1; i < argc; ++i ) {
	if ( argv[i][0] == '-' || argv[i][0] == '/' ) {
		switch ( argv[i][1] ) {
		   case 'e':
			eval_debug = TRUE;
			break;
		}
	}
   }
}

/* ----------------------------------------------------------------------- */
/*                         Environment Functions			   */
/* ----------------------------------------------------------------------- */

/* evDefGlobal( sym, val ) - Bind the global variable sym to val. */
void evDefGlobal( sym, val )
CONS sym, val;
{
   if ( !mcSymbol(sym) ) {
	RT_LERROR("Non-symbol passed to evDefGlobal(): ", sym);
   }

   *mcVect_Ref( mcGet_Global(glo_env), mcGet_Int(sym) ) = val;
}

/* evAccNested(sym, env) -- Return the nested binding for sym in env.
	Returns NIL if no binding exists.  NOTE: This function returns
	A-LIST dotted-pairs!
*/
CONS evAccNested(sym, env)
CONS sym, env;
{
   return mcQAssoc( sym, mcGet_Nested(env) );
}

/* evAccGlobal(sym, env) - Return the global binding of sym in env.
	Returns NULL if the symbol isn't bound.  NOTE: This function
	returns the actual value, NOT a dotted-pair.
*/
CONS evAccGlobal(sym, env)
CONS sym, env;
{
   return *mcVect_Ref( mcGet_Global(env), mcGet_Int(sym) );
}

/* evSaveEnv() - Saves the current environment so it can be restored after
	the user function or form has completed.
*/
void evSaveEnv()
{
#ifdef NO_TAIL_RECURSION
   /* save the current environment, to restore after the closure */
   mcPushExpr( mcGet_Nested(glo_env) );
   mcPushExpr( RESTORE );
#else
   /* save the current environment, to restore after the closure */
   if ( mcExprStackTop() != RESTORE && !mcExe( mcExprStackTop() ) ) {
	/* if there is already an environment on the stack, don't push
	 * another one.  this handles tail recursion and keeps the stack
	 * smaller since we won't be pushing ALL of the intermediate
	 * environments, just the environment to return to when we're all
	 * done.
	 */
	mcPushExpr( mcGet_Nested(glo_env) );
	mcPushExpr( RESTORE );
   }
#endif
}

/* ----------------------------------------------------------------------- */
/*                          Compiler Functions				   */
/* ----------------------------------------------------------------------- */

/* evAddFunc(pr, f) -- Add a Scheme operation to the byte-code function
	lookup table.  n is the predefined # and f is the name of the
	function.
*/
void evAddFunc(pr, f)
int pr;
void (*f)( C_VOID );
{
   assert( pr < NUM_FUNCS );
   BOPS[pr] = f;
}

/* ----------------------------------------------------------------------- */
/*                            Call a function				   */
/* ----------------------------------------------------------------------- */

/* evCallFunc(f,a) - Setup to call the function f with args a.  Operations
	call this function when they want to invoke a function.
*/
void evCallFunc(func, args)
CONS func, args;
{
   int nargs;
   CONS sargs;

   mcPushExpr( CALL );
   mcPushFunc( func );

   /* determine whether or not a MARK is needed */
   if ( mcFunc(func) ) {
	/* invoking a system function; make sure the # of args is
	 * correct and push a MARK only if it's a var-args.
	 */
	nargs = mcLength(args);

	if ( (mcPrim_RA(func) == nargs ) ||
	     (mcPrim_AA(func) >= mcPrim_RA(func) && nargs == mcPrim_AA(func)) ||
	     (mcPrim_AA(func) < mcPrim_RA(func)  && nargs >= mcPrim_RA(func)) ) {
		/* if it's var-args then push a MARK */
		if ( mcPrim_RA(func) != mcPrim_AA(func) )
			mcPushVal( MARK );
	} else {
		/* wrong # of args */
		fprintf(currout, "\nError: EVAL: Wrong # of args to primitive procedure %s: ", mcPrim_Name(func));
		fprintf(currout, "\n\n");
		ERROR;
	}
   }
   else if ( !mcCont(func) ) {
	/* invoking a user-defined function; push a MARK */
	mcPushVal( MARK );
   }

   /* push the args */
   if ( mcAtom(args) )
	mcPushVal( args );
   else {
	/* need to reverse the arg-list so args are on the
	 * stack in the right order
	 */
	sargs = args = mcRev(args);
	while ( !mcNull(args) ) {
		mcPushVal( mcCar(args) );
		args = mcCdr( args );
	}
	sargs = mcRev(sargs);
   }
}

/* ----------------------------------------------------------------------- */
/*                          Argument Functions				   */
/* ----------------------------------------------------------------------- */

/* evCountArgs() - Count the # of args on the expression stack. */
static void evCountArgs(func)
CONS func;
{
   int num;
   CONS *curr;

   assert( mcFunc(func) || mcForm(func) );

   num = 0;
   for ( curr = Top_Expr; curr > ExprStack && *curr != CALL ; --curr )
	++num;

   if ( (mcPrim_RA(func) == num ) ||
	(mcPrim_AA(func) >= mcPrim_RA(func) && num == mcPrim_AA(func)) ||
	(mcPrim_AA(func) < mcPrim_RA(func)  && num >= mcPrim_RA(func)) )
	return;

   /* wrong # of args */
   fprintf(currout, "\nError: EVAL: Wrong # of args to primitive procedure %s: ", mcPrim_Name(func));
   fprintf(currout, "\n\n");
   ERROR;
}

/* evGatherVal() - Gathers the arguments on the ValStack into a list.  There
	better be a MARK on the val stack.
*/
CONS evGatherVal()
{
   ENTER;
   REG(exp);
   REG(args);

   R(args) = NIL;
   R(exp) = mcPopVal();
   while ( R(exp) != MARK ) {
	R(args) = mcCons( R(exp), R(args) );
	R(exp) = mcPopVal();
   }

   /* have to reverse the list */
   R(args) = mcRev( R(args) );

   MCLEAVE R(args);
}

/* evGatherExpr() - Gathers the arguments on the ExprStack into a list.  There
	better be a CALL on the expr stack.
*/
CONS evGatherExpr()
{
   ENTER;
   REG(exp);
   REG(args);

   R(args) = NIL;
   R(exp) = mcPopExpr();
   while ( R(exp) != CALL ) {
	R(args) = mcCons( R(exp), R(args) );
	R(exp) = mcPopExpr();
   }

   MCLEAVE R(args);
}

/* ----------------------------------------------------------------------- */
/*                         Invoke User Functions			   */
/* ----------------------------------------------------------------------- */

/* evInvokeUserFunc( parms, body, env ) - Invokes the user-defined
	code (either special form or closure) by:

	(1) Saving the current environment.
	(2) Binding the arguments to the parameters.
	(3) Pushing the body on the ExprStack. (Evaluate the body)

	The arguments are on the value stack.
*/
static void evInvokeUserFunc( parms, body, env )
CONS parms, body, env;
{
   EV_DEBUG("\nIn evInvokeUserFunc, parms = ", parms);
   EV_DEBUG("\n\tbody = ", body );

   /* (2) Bind args to parms (extend the environment) */
   evSaveEnv();
   mcGet_Nested(glo_env) = evBindArgs( parms, env );

   /* (3) Evaluate the body. */
   if ( mcExe(body) || mcCode(body) ) {
	/* invoke the byte-code interpreter */
	evInvokeBC(body);
   } else {
	/* use begin to handle the body of the func,
   	 * push the body onto the expr stack so it is the arg to opBegin()
	 */
	mcPushExpr( CALL );
	while ( !mcNull(body) ) {
		mcPushExpr( mcCar(body) );
		body = mcCdr( body );
	}
	opBegin();
   }
}

/* evBindArgs(parms, env) - This is to bind the paramters to the arguments for
	user defined functions.  The args are popped off of the value stack.
	Returns the extended environment.

	NOTATION:

	(lambda parm-spec body)
*/
static CONS evBindArgs(p, e)
CONS p, e;
{
   ENTER;
   REG(var);
   REG(value);
   REG(n_env);
   REG(parms);

   R(parms) = p;
   R(n_env) = e;

   EV_DEBUG("\nIn evBindArgs, parms = ", p);

   /* Bind arguments to the parameters. */
   while ( !mcNull( R(parms) ) ) {

	/* handle atom parm-spec && improper list */
	if ( mcAtom( R(parms) ) ) {
		/* gather the rest of the args into a list, and bind this
		 * list to the last parameter.
		 */
		R(var) = evGatherVal();
		R(var) = mcCons( R(parms), R(var) );

		/* add binding to environment */
		R(n_env) = mcCons( R(var), R(n_env) );

		/* done binding */
		MCLEAVE R(n_env);
	}

	/* get the next argument */
	R(value) = mcPopVal();

	/* run out of arguments? */
	if ( R(value) == MARK )
		break;

	/* strip off first variable name */
	R(var) = mcCar( R(parms) );
	R(parms) = mcCdr( R(parms) );

	/* make binding */
	R(var) = mcCons( R(var), R(value) );

	/* add binding to environment */
	R(n_env) = mcCons( R(var), R(n_env) );
   }

   /* #parms == #args */
   if ( R(value) == MARK ) {
	RT_ERROR("Too few args in call to function.");
   }
   else if ( mcValStackTop() != MARK  ) {
	RT_ERROR("Too many args in call to function.");
   }

   /* pop MARK off the value stack */
   (void)mcPopVal();

   EV_DEBUG("\nBound args.\n", NIL);
   MCLEAVE R(n_env);
}

/* ----------------------------------------------------------------------- */
/*                           Invoke User Forms				   */
/* ----------------------------------------------------------------------- */

/* evInvokeUserForm() - Invokes a user defined special form. */
static void evInvokeUserForm(parms, body)
CONS parms, body;
{

   EV_DEBUG("\nIn evInvokeUserForm, parms = ", parms);
   EV_DEBUG("\n\tbody = ", body );

   /* Bind args to parms (extend the environment) */
   evSaveEnv();
   mcGet_Nested(glo_env) = evBindFormArgs( parms, mcGet_Nested(glo_env) );

   /* Evaluate the body. */
   if ( mcExe(body) || mcCode(body) ) {
	/* invoke the byte-code interpreter */
	evInvokeBC(body);
   } else {
	/* use begin to handle the body of the form,
   	 * push the body onto the expr stack so it is the arg to opBegin()
	 */
	mcPushExpr( CALL );
	while ( !mcNull(body) ) {
		mcPushExpr( mcCar(body) );
		body = mcCdr( body );
	}
	opBegin();
   }
}

/* evBindFormArgs(parms, env) - This is to bind the paramters to the arguments
	for user defined forms.  The args are popped off of the expr stack.
	Returns the extended environment.
*/
static CONS evBindFormArgs(p, e)
CONS p, e;
{
   ENTER;
   REG(var);
   REG(value);
   REG(n_env);
   REG(parms);

   R(parms) = p;
   R(n_env) = e;

   EV_DEBUG("\nIn evBindFormArgs, parms = ", p);

   /* Bind arguments to the parameters. */
   while ( !mcNull( R(parms) ) ) {

	/* handle atom parm-spec && improper list */
	if ( mcAtom( R(parms) ) ) {
		/* gather the rest of the args into a list, and bind this
		 * list to the last parameter.
		 */
		R(var) = evGatherExpr();
		R(var) = mcCons( R(parms), R(var) );

		/* add binding to environment */
		R(n_env) = mcCons( R(var), R(n_env) );

		/* done binding */
		MCLEAVE R(n_env);
	}

	/* get the next argument */
	R(value) = mcPopExpr();

	/* run out of arguments? */
	if ( R(value) == CALL )
		break;

	/* strip off first variable name */
	R(var) = mcCar( R(parms) );
	R(parms) = mcCdr( R(parms) );

	/* make binding */
	R(var) = mcCons( R(var), R(value) );

	/* add binding to environment */
	R(n_env) = mcCons( R(var), R(n_env) );
   }

   /* #parms == #args */
   if ( !mcNull( R(parms) ) || R(value) != CALL )
	RT_ERROR("Wrong number of args in call to form.");

   MCLEAVE R(n_env);
}

/* ----------------------------------------------------------------------- */
/*                         Invoke Continuations				   */
/* ----------------------------------------------------------------------- */

/* evInvokeCont(c) - Invokes the continuation by restoring the ExprStack,
	the ValStack, the FuncStack, and the environment to what they were
	when the continuation was captured.
*/
static void evInvokeCont(c)
CONS c;
{
   ENTER;
   REG(val);

   assert( mcCont(c) );

   /* value to return */
   R(val) = mcPopVal();

   /* restore the stacks */
   mcRestExpr( mcCont_Exp(c) );
   mcRestVal( mcCont_Val(c) );
   mcRestFunc( mcCont_Fnc(c) );

   /* restore environment */
   mcGet_Nested(glo_env) = mcCont_Env(c);

   mcPushVal( R(val) );
   MCLEAVE;
}

/* ----------------------------------------------------------------------- */
/*                         Invoke Special Forms				   */
/* ----------------------------------------------------------------------- */

/* evMkResume(pr) - Makes and returns a resume with the specified pr
	# in it.
*/
CONS evMkResume(pr)
int pr;
{
   CONS res;

   res = NewCons( RESUME, 0, 0 );
   mcCpy_Int( res, pr );
   return res;
}

/* evInvokeForm(f) - Invoke the special form f. */
static void evInvokeForm(f)
CONS f;
{
   ENTER;
   REG(form);
   R(form) = f;

   /* make sure the user really is invoking a special form */
   if ( mcExprStackTop() != PUSHFUNC ) {
	MCLEAVE;
   }

   /* pop the PUSHFUNC off the expression stack */
   mcPopExpr();

   /* user defined special form? */
   if ( mcUserForm(f) ) {
	EV_DEBUG("\n\tIn evInvokeForm, invoking user special form.", NIL);

	/* special forms are evaluated in the current environment NOT the
	 * environment that the form was defined in.
	 */
	evInvokeUserForm( mcForm_Parms(f), mcForm_Body(f) );
	MCLEAVE;
   }

   /* evaluating a primitive special form */
   assert( mcForm(f) );

   EV_DEBUG("\nIn evInvokeForm, invoking form.", NIL);

   /* make sure there are the correct # of args */
   evCountArgs(f);

   /* invoke the special form */
   (*mcPrim_Op(f))();
   MCLEAVE;
}

/* ----------------------------------------------------------------------- */
/*                      Invoke Interpreter Resumes			   */
/* ----------------------------------------------------------------------- */

/* evInvokeRes(f) - Resume a function or form which terminated because it
	needed to evaluate something.
*/
static void evInvokeRes(f)
CONS f;
{
   int pr;

   /* extract the pr number of the operation to resume */
   pr = mcGet_Int(f);

   if ( eval_debug )
	printf("\n\t In evInvokeRes, PRNUM= %d", pr);

   switch ( pr ) {

	case prDefine:
		opResDefine();
		break;

	case prSet:
		opResSet();
		break;

	case prIf:
		opResIf();
		break;

	case prBegin:
		opResBegin(f);
		break;

	case prOr:
		opResOr(f);
		break;

	case prAnd:
		opResAnd(f);
		break;

	case prMacro:
		opResMacro();
		break;

	case prmcExpand:
		evResExpand();
		break;

	case prLoad:
		(void)mcResLoad(f);
		break;

	default:
		printf("Illegal pr # in resume: %d.\n", pr);
		assert(0);
   }
}

/* ----------------------------------------------------------------------- */
/*                                 EVAL					   */
/* ----------------------------------------------------------------------- */

/* evEvalAtom( atom, env ) - Returns the evaluation of the atom in the
	current evironment.
*/
static CONS evEvalAtom( atom )
CONS atom;
{
   CONS binding;

   /* (eval #T) => #T && (eval #F) => #F && (eval number) => number */
   if ( atom == T  || atom == F || mcNumber(atom) )
	return atom;

   /* (eval string) => string && (eval char) => char */
   if ( mcString(atom) || mcChar(atom) || mcVector(atom) )
	return atom;

   if ( !mcSymbol(atom) )
	RT_LERROR("EVAL: Can't evaluate non-symbol: ", atom);

   /* (eval symbol) => symbol's-binding */
   binding = evAccNested( atom, glo_env );
   if ( !mcNull(binding) ) {
	/* symbol's-binding = cdr(binding) [remember they're dps!] */
	binding = mcCdr( binding );
	return binding;
   }
   else if ( (binding = evAccGlobal( atom, glo_env )) != NULL ) {
	return binding;
   }

   RT_LERROR("EVAL: Undefined symbol ", atom);

   /*NOTREACHED!*/
   /* please the ANSI compiler */
   return NIL;
}

/* evEval() - Evaluate the expression on TOP of the expression stack in the
	given environment.  Returns the evaluation on the value stack.
*/
void evEval()
{
   ENTER;

   REG(exp);		/* expression to evaluate */
   REG(func);		/* function to invoke */

   /* Keep evaluating until there are no more expressions to evaluate */
   while ( mcHaveExprs() ) {

	/* dump stacks for debugging purposes */
	if ( eval_debug )
		mcDumpStacks();

	/* evaluate the expression on top of the expression stack */
	R(exp) = mcPopExpr();

	EV_DEBUG("\nExp to eval = ", R(exp) );

	if ( R(exp) == NIL ) {
		/* the Scheme standard says that () is an ILLEGAL SYNTAX
		 * form, but for compatability with other Scheme's
		 * (eval ()) => ()
		 */
		mcPushVal( R(exp) );
	}
	else if ( R(exp) == PUSHFUNC ) {
		/* pop function off the value stack,
		 * push it onto the function stack,
		 * if it's a primitive var-args function then push MARK
		 */
		R(func) = mcPopVal();
		mcPushFunc( R(func) );

		if ( mcFunc(R(func)) ) {
			/* make sure there are the correct # of args */
			evCountArgs( R(func) );

			/* if it's var-args then push a MARK */
			if ( mcPrim_RA(R(func)) != mcPrim_AA(R(func)) )
				mcPushVal( MARK );
		} else {
			/* user defined functions MUST have a MARK.  the
			 * MARK is popped off in evBindArgs()
			 */
			mcPushVal( MARK );
		}
	}
	else if ( R(exp) == CALL ) {
		/* pop the function off the function stack, and
		 * apply it.
		 */
		R(func) = mcPopFunc();

		EV_DEBUG("\nInvoking func: ", R(func) );

		/* apply func to it's arguments */
		evApply( R(func) );
	}
	else if ( mcCode(R(exp)) || mcExe(R(exp)) ) {
		/* executing byte-code */
		evInvokeBC( R(exp) );
	}
	else if ( R(exp) == RESTORE ) {

		EV_DEBUG("\tRestoring previous environment.", NIL);

		/* restore previous environment */
		mcGet_Nested(glo_env) = mcPopExpr();
	}
	else if ( mcResume( R(exp) ) ) {

		/* resume a func or form */
		evInvokeRes( R(exp) );
	}
	else if ( mcAtom( R(exp) ) ) {

		/* evaluate an atom */
		R(exp) = evEvalAtom( R(exp) );

		/* invoke a special form before it's args are
		 * evaluated.
		 */
		if ( mcForm( R(exp) ) || mcUserForm( R(exp) ) )
			evInvokeForm( R(exp) );
		else mcPushVal( R(exp) );
	}
	else {
		/* evaluate list of expressions */

		EV_DEBUG("\nIn mcEval, evaluating list of expressions.", NIL);

		/* does the current expression need to be expanded?  if so,
		 * we need to skip evaluating it.
		 */
		if ( evExpandOnce( R(exp) ) )
			continue;

		/* mark place on expr stack */
		mcPushExpr( CALL );

		/* separate function from args  */
		R(func) = mcCar( R(exp) );
		R(exp) = mcCdr( R(exp) );

		/* push args onto expr stack */
		while ( !mcNull( R(exp) ) ) {
			mcPushExpr( mcCar( R(exp) ) );
			R(exp) = mcCdr( R(exp) );
		}

		/* push PUSHFUNC so function will be moved to func
		 * stack after it's evaluated.
		 */
		mcPushExpr( PUSHFUNC );

		/* push function/form to be evaluated */
		mcPushExpr( R(func) );
	}
   }
   MCLEAVE;
}

/* ----------------------------------------------------------------------- */
/*                                 APPLY				   */
/* ----------------------------------------------------------------------- */

/* evApply(func) - Applies the function to it's arguments in the
	current environment.  The arguments are on the value stack.
*/
void evApply(func)
CONS func;
{
   EV_DEBUG( "\nIn evApply, func = ", func );

   if ( mcClosure(func) ) {
	/* invoke a user defined function */
	EV_DEBUG("\n\tIn evApply, calling evInvokeUserFunc.", NIL);
	evInvokeUserFunc( mcCl_Parms(func), mcCl_Body(func), mcCl_Env(func) );
	return;
   }

   if ( mcCont(func) ) {
	/* invoking a continuation */
	EV_DEBUG("\n\t\t\tIn evApply, calling evInvokeCont.", NIL);
	evInvokeCont(func);
	return;
   }

   /* func better be a primitive function */
   if ( !mcFunc(func) )
	RT_LERROR("APPLY: Can't apply the non-function ", func);

   /* invoke the primitive function; the # of args has already been checked
    * by evEval().
    */
   (*mcPrim_Op(func))();
}

/* ----------------------------------------------------------------------- */
/*                         Byte-code Interpreter			   */
/* ----------------------------------------------------------------------- */

/* evInvokeBC() - Interpret compiled scheme expressions.  (Byte-code) */
static void evInvokeBC(bc)
CONS bc;
{
   CONS sym, temp;
   unsigned char op;
   unsigned int pc;

   if ( mcExe(bc) ) {
	/* execution point => restoring environment, byte-code, and pc */
	mcGet_Nested(glo_env) = mcExe_Env(bc);
	pc = mcExe_PC(bc);
	bc = mcExe_BC(bc);
   } else {
	assert( mcCode(bc) );
	pc = 0;
   }

   /* execute the byte-code */
   while ( pc < mcBC_CSize(bc) ) {

	/* fetch the next instruction */
	op = *(mcBC_Code(bc)+pc);

	if ( eval_debug ) {
		printf("Byte-code op: %d", op);
		printf("\tPC: %d\tSize: %d\n", pc, mcBC_CSize(bc));
		mcDumpStacks();
		printf("\n");
	}

	/* increment to next instruction */
	++pc;

	/* evaluate it */
	switch (op) {
	   case prNoOp:
		break;

	   case prCollectArgs:
		temp = evGatherVal();
		mcPushVal(temp);
		break;

	   case prPushConst:
		op = *(mcBC_Code(bc)+pc);
		temp = *(mcBC_Const(bc)+op);
		mcPushVal(temp);

		/* increment pc beyond constant table pntr */
		++pc;
		break;

	   case prPushVar:
		op = *(mcBC_Code(bc)+pc);
		sym = *(mcBC_Const(bc)+op);

		temp = evAccNested( sym, glo_env );
		if ( !mcNull(temp) )
			mcPushVal( mcCdr(temp) );
		else if ( (temp = evAccGlobal( sym, glo_env )) != NULL )
			mcPushVal( temp );
		else {
			RT_LERROR("EVAL: Undefined symbol ", sym);
		}

		/* increment pc beyond constant table pntr */
		++pc;
		break;

	   case prReturn:
		/* forced return from byte-code */
		return;

	   case prNilBranch:
		temp = mcPopVal();
		if ( mcNull(temp) || temp == F )
			pc = *(mcBC_Code(bc)+pc);
		else
			/* increment pc beyond address to branch to */
			++pc;

		break;

	   case prBranch:
		pc = *(mcBC_Code(bc)+pc);
		break;

	   case prPopVal:
		/* pop closure off val stack and push onto func stack */
		temp = mcPopVal();
		mcPushFunc( temp );
		break;

	   case prMakeClosure:
		opMakeClosure();
		break;

	   case prPushMark:
		mcPushVal( MARK );
		break;

	   case prPushFunc:
		/* pop func off val stack and push onto func stack */
		temp = mcPopVal();
		mcPushFunc( temp );
		break;

	   case prCall:
		/* invoke a compiled user function */
		evSaveExe(pc,bc);
		mcPushExpr( CALL );

		return;

	   default:
		/* eval, apply, and call/cc require a break from the
		 * byte-code to perform an evaluation -- use bcCall to
		 * setup an execution point.
		 */
		if ( op == prEval || op == prApply || op == prCallCC )
			evSaveExe(pc,bc);

		/* invoke the primitive function */
		(*BOPS[op])();

		/* eval, apply, and call/cc require a break from the
		 * byte-code to perform an evaluation.
		 */
		if ( op == prEval || op == prApply || op == prCallCC )
			return;

		break;
	}
   }
}

/* evSaveExe(pc,bc) - Certain byte-code operations invoke 'eval'.  This
	routine saves the execution point so that 'eval' will return
	here.
*/
static void evSaveExe(pc,bc)
int pc;
CONS bc;
{
   /* eliminate tail-recursion: if we're done in this byte-code then don't
    * push an execution point.  the call will return directly to
    * this byte-code's caller.
    */
   if ( pc < mcBC_CSize(bc) && *(mcBC_Code(bc)+pc) != prReturn ) {
	/* not a tail-recursive call; we'll be returning to this byte-code
	 * so create an execution point
	 */
	CONS exepnt;

	exepnt = NewCons( EXEPOINT, 0, 0 );
	mcExe_BC(exepnt) = bc;
	mcExe_PC(exepnt) = pc;
	mcExe_Env(exepnt) = mcGet_Nested(glo_env);

	mcPushExpr(exepnt);
   }
}

/* ----------------------------------------------------------------------- */
/*                      Interpreter System Expander			   */
/* ----------------------------------------------------------------------- */

/* evExpandOnce(e) - Expand the given expression.  Returns TRUE if an
	expansion is required.  Returns FALSE if no expansion is being
	performed.  Works by looking up the car of the expression in the
	expansion a-list.
*/
static int evExpandOnce(exp)
CONS exp;
{
   CONS etbl;
   CONS binding;	/* no need for a register since it points into glo_env */

   /* no need to expand if the car isn't a symbol */
   if ( mcPair( mcCar(exp) ) ) {
	return FALSE;
   }

   /* get the expansion table */
   etbl = evAccGlobal( EXP_TABLE, glo_env );
   if ( mcNull( etbl ) )
	return FALSE;

   /* exp is a list of the form (func arg1 ...), lookup func in table */
   binding = mcQAssoc( mcCar(exp), etbl );
   if ( !mcNull( binding ) ) {
	/* have to invoke the expander function on the exp */
	mcPushExpr( EXP_RESUME );
	mcPushExpr( CALL );

	mcPushVal( MARK );
	mcPushVal( exp );

	mcPushFunc( mcCdr(binding) );
	return TRUE;
   }

   return FALSE;
}

/* evResExpand() - Resume expanding by popping the expander function's
	result off ValStack and pushing it on ExprStack to be evaluated.
*/
static void evResExpand()
{
   ENTER;
   REG(result);
   R(result) = mcPopVal();
   mcPushExpr( R(result) );
   MCLEAVE;
}

/* ----------------------------------------------------------------------- */
/*                          Debugging Routines				   */
/* ----------------------------------------------------------------------- */

/* evError() - This function gets called when there is an evaluation error.
	It can either jump to the top-level or dump the stacks.
*/
void evError()
{
   mcDumpStacks();
   fprintf(currout, "\nReturning to top-level.\n");
   longjmp(tlevel,1);
}

static void mcDumpStack(top, stack)
CONS *top, stack[];
{
   int l;
   CONS *et;

   if ( top <= stack )
	printf("  <EMPTY>\n");
   else {
	for ( l = 0, et = top; l < 5 && et > stack; ++l, --et ) {
		mcWrite( R(et), currout );
		printf(" | ");
	}
	printf("\n");
   }
}

/* mcDumpStacks() - Dumps the expression and value stacks. */
static void mcDumpStacks()
{
   printf("\n");

   /* dump expression stack */
   printf("Expression stack: ");
   mcDumpStack( Top_Expr, ExprStack );

   /* dump value stack */
   printf("Value stack: ");
   mcDumpStack( Top_Val, ValStack );

   /* dump function stack */
   printf("Function stack: ");
   mcDumpStack( Top_Func, FuncStack );
}
