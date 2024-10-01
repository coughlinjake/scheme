/* microcode.c - the C, low-level functions.

   Optimizations:
	- mcCar, mcCdr can be implemented as macros.

   Version 1
	- Lowest level C routines which manipulate the structures directly.

	- ACTIVE - Node will be marked during GC and WON'T disappear.
	  INACTIVE - Node is unknown to GC and will disappear next GC.

	- Register variables are used to ACTIVATE nodes:

	   ENTER  - marks the register stack-top.
	   REG(x) - makes x an ACTIVATED cons node by pushing it on the
		register stack.  REGs declared before ENTER will remain
		after LEAVE.
	   R(x)   - accesses the ACTIVATED cons node x.
	   LEAVE  - restores the register stack to the previous state.  REGs
		declared after ENTER disappear.

	- Credit for the above goes to Gary Levin from his ISETL source
	code.

	- Parameters to micro-code routines should be ACTIVE.  Results of
	  micro-code routines may be INACTIVE!

   BUGS:
	- mcTree_Copy() should be changed to recur only on the car's and
	iterate on the cdr's so we don't blow out the C stack.

*/

#include "machine.h"

#include MEMORY_H
#include STRING_H

#include "glo.h"
#include "micro.h"
#include "eval.h"
#include "memory.h"
#include "error.h"
#include "scanner.h"
#include "symstr.h"
#include "predefs.h"

/* globals */
CONS RegStack[MAX_REGSTACK];		/* register stack */
CONS ExprStack[MAX_EXPRSTACK];		/* expression stack */
CONS ValStack[MAX_VALSTACK];		/* value stack */
CONS FuncStack[MAX_FUNCSTACK];		/* the function stack */
CONS *Top_RegS;				/* top of the register stack */
CONS *SavedRegs;			/* saved register stack */
CONS *Top_Expr;				/* top exp on exp stack */
CONS *Top_Val;				/* top val on value stack */
CONS *Top_Func;				/* top func on function stack */

/* system constants */
CONS NIL;				/* empty list */
CONS T;					/* Scheme: Boolean true value */
CONS F;					/* Scheme: Boolean false value */
CONS EOF_OBJ;				/* EOF object in Scheme */
CONS STDIN;				/* port for stdin */
CONS STDOUT;				/* port for stdout */
CONS CALL;				/* call the function */
CONS MARK;				/* mark the expr stack */
CONS PUSHFUNC;				/* push func onto the func stack */
CONS RESTORE;				/* restore the environment */
CONS EXP_RESUME;			/* resume mcExpandOnce() */
CONS EXP_TABLE;				/* symbol: *EXPANSION-TABLE* */

/* local prototypes */
static void mcNewStacks( C_VOID );
static CONS mcDefConst( C_CHAR C_PTR );

/* InitMicro() - Initializes the microcode.
	- initialize the stacks
	- create needed system variables (NIL, T, CALL, MARK, etc ...)
*/
void InitMicro()
{
   /* create NIL */
   NIL = NewCons( NILNODE, 0, 0 );

   /* create the stacks */
   mcNewStacks();

   mcRegPush(NIL);		/* put on register stack so not GCed */

   /* create other Scheme objects */
   T = NewCons( TOBJ, 0, 0 );
   mcRegPush( T );
   F = NewCons( FOBJ, 0, 0 );
   mcRegPush( F );
   EOF_OBJ = NewCons( EOFOBJ, 0, 0 );
   mcRegPush( EOF_OBJ );

   /* create system variables */
   CALL = mcDefConst( "*CALL*" );
   MARK = mcDefConst( "*MARK*" );
   PUSHFUNC = mcDefConst( "*PUSHFUNC*" );
   RESTORE = mcDefConst( "*RESTORE*" );
   EXP_TABLE = mcDefConst( "*EXPANSION-TABLE*" );

   /* this one depends on the fact that mcCons doesn't lose its
    * args.  not supposed to be making assumptions like that.
    */
   EXP_RESUME = evMkResume( prmcExpand );
   mcRegPush( EXP_RESUME );

   STDIN = NewCons( PORT, 0, 0 );
   mcCpy_Port(STDIN, stdin);
   mcCpy_PortType( STDIN, INPUT );
   mcRegPush(STDIN);

   STDOUT = NewCons( PORT, 0, 0 );
   mcCpy_Port(STDOUT, stdout);
   mcCpy_PortType( STDOUT, OUTPUT );
   mcRegPush(STDOUT);

   /* save the register stack so we can get the system vars back after an
    * error causes the stacks to be cleared.
    */
   SavedRegs = Top_RegS;
}

/* mcDefConst(n) - Define a system constant whose name is n.  Saves the
	constant on the register stack AND returns it.
*/
static CONS mcDefConst(n)
char *n;
{
   CONS tmp;

   tmp = NewCons( SYMBOL, 0, 0 );
   mcCpy_Sym(tmp, n);
   mcRegPush(tmp);
   return tmp;
}

/* ----------------------------------------------------------------------- */
/*                            Stack Routines				   */
/* ----------------------------------------------------------------------- */

/* mcNewStacks() - Clears the stacks entirely. */
static void mcNewStacks()
{
   Top_Expr = ExprStack;
   Top_Val  = ValStack;
   Top_Func = FuncStack;
   Top_RegS = RegStack;		/* totally clear the register stack */
}

/* mcClearStacks() - Clears the expression and value stacks.  Restores the
	register stack to contain just the system variables -- can't EVER
	totally clear this stack or else you'll lose the definitions for
	NIL, T, MARK, etc.
*/
void mcClearStacks()
{
   Top_RegS = SavedRegs;	/* restore register stack (system vars!) */
   Top_Expr = ExprStack;
   Top_Val  = ValStack;
   Top_Func = FuncStack;
}

/* mcPushExpr(c) */
void mcPushExpr(c)
CONS c;
{
   if ( Top_Expr >= &ExprStack[MAX_EXPRSTACK-1] ) {
	RT_ERROR("Expression stack overflow.");
   }

   R(++Top_Expr) = c;
}

/* mcPushVal(c) */
void mcPushVal(c)
CONS c;
{
   if ( Top_Val >= &ValStack[MAX_VALSTACK-1] ) {
	RT_ERROR("Value stack overflow.");
   }

   R(++Top_Val) = c;
}

/* mcPushFunc(c) */
void mcPushFunc(c)
CONS c;
{
   if ( Top_Func >= &FuncStack[MAX_FUNCSTACK-1] ) {
	RT_ERROR("Function stack overflow.");
   }

   R(++Top_Func) = c;
}

/* mcPopExpr() - Returns and pops the top of the expression stack. */
CONS mcPopExpr()
{
   CONS temp;

   if ( Top_Expr <= ExprStack ) {
	RT_ERROR("Expression stack underflow.");
   }

   temp = R(Top_Expr);
   --Top_Expr;
   return temp;
}

/* mcPopVal() - Returns and pops the top of the value stack. */
CONS mcPopVal()
{
   CONS temp;

   if ( Top_Val <= ValStack ) {
	RT_ERROR("Value stack underflow.");
   }

   temp = R(Top_Val);
   --Top_Val;
   return temp;
}

/* mcPopFunc() - Returns and pops the top of the function stack. */
CONS mcPopFunc()
{
   CONS temp;

   if ( Top_Func <= FuncStack ) {
	RT_ERROR("Function stack underflow.");
   }

   temp = R(Top_Func);
   --Top_Func;
   return temp;
}

/* mcGetExprS() - Captures the expression stack. */
CONS mcGetExprS()
{
   CONS *curr;

  {
   ENTER;
   REG(cap);

   R(cap) = NIL;

   for ( curr = Top_Expr; curr > ExprStack; --curr )
	R(cap) = mcCons( R(curr), R(cap) );

   MCLEAVE R(cap);
  }
}

/* mcGetValS() - Captures the value stack. */
CONS mcGetValS()
{
   CONS *curr;

  {
   ENTER;
   REG(cap);

   R(cap) = NIL;

   for ( curr = Top_Val; curr > ValStack; --curr )
	R(cap) = mcCons( R(curr), R(cap) );

   MCLEAVE R(cap);
  }
}

/* mcGetFuncS() - Captures the function stack. */
CONS mcGetFuncS()
{
   CONS *curr;

  {
   ENTER;
   REG(cap);

   R(cap) = NIL;

   for ( curr = Top_Func; curr > FuncStack; --curr )
	R(cap) = mcCons( R(curr), R(cap) );

   MCLEAVE R(cap);
  }
}

/* mcRestExpr(c) - Restore expression stack c. */
void mcRestExpr(c)
CONS c;
{
   CONS head;

   Top_Expr = ExprStack;
   while ( !mcNull(c) ) {
	head = mcCar(c);
	c = mcCdr(c);

	R(++Top_Expr) = head;
   }
}

/* mcRestVal(c) - Restore value stack c. */
void mcRestVal(c)
CONS c;
{
   CONS head;

   Top_Val = ValStack;
   while ( !mcNull(c) ) {
	head = mcCar(c);
	c = mcCdr(c);

	R(++Top_Val) = head;
   }
}

/* mcRestFunc(c) - Restore function stack c. */
void mcRestFunc(c)
CONS c;
{
   CONS head;

   Top_Func = FuncStack;
   while ( !mcNull(c) ) {
	head = mcCar(c);
	c = mcCdr(c);

	R(++Top_Func) = head;
   }
}

/* ----------------------------------------------------------------------- */
/*                         Environment Routines				   */
/* ----------------------------------------------------------------------- */

/* mcMkEnv() - Returns a new environment. */
CONS mcMkEnv()
{
   ENTER;
   REG(env);

   R(env) = NewCons( ENVMNT, 0, 0 );
   mcGet_Nested( R(env) ) = NIL;
   mcGet_Global( R(env) ) = NewCons( VECTOR, MAX_SYMBOLS, 0 );

   /* fill vector with NULL's since this environment doesn't have
    * any bindings yet.
    */
   mcVectorFill( mcGet_Global(R(env)), NULL );

   MCLEAVE R(env);
}

/* ----------------------------------------------------------------------- */
/*                            Hash Functions				   */
/* ----------------------------------------------------------------------- */

/* mcHash(s, size) - Returns the hash value of the symbol s.  size is the
	maximum size of the hash table.
*/
int mcHash(s, size)
char *s;
int size;
{
   unsigned int i, h;

   h = 0;
   for ( i = 1; *s != EOS; i++, s++)
	h = h + (i * (*s));

   return ((int)(h % size));
}

/* mcRehash(s, h) - Rehashes the symbol s.  h is the old hash value.
	NOTE: s not used at this time, but the algorithm might change and
	this makes it easier to upgrade.
*/
int mcRehash(s, h, size)
char *s;
int h, size;
{
   return ((h+1) % size);
}

/* ----------------------------------------------------------------------- */
/*                       Primitive List Operations			   */
/* ----------------------------------------------------------------------- */

/* mcIntToCons(i) - Creates and returns a CONS node with the integer i
	in it.
*/
CONS mcIntToCons(i)
int i;
{
   CONS temp;

   temp = NewCons( INT, 0, 0 );
   mcCpy_Int(temp, i);
   return temp;
}

/* mcCharToCons(c) - Creates and returns a CONS node with the character c
	in it.
*/
CONS mcCharToCons(c)
int c;
{
   CONS temp;

   if ( c == EOF )
	return EOF_OBJ;

   temp = NewCons( CHAR, 0, 0 );
   mcCpy_Char(temp, (char)c);
   return temp;
}

/* mcCadr(l) - Returns the CADR of the list l. */
CONS mcCadr(l)
CONS l;
{
   l = mcCdr(l);
   l = mcCar(l);
   return l;
}

/* mcCaadr(l) - Returns the CAADR of the list l. */
CONS mcCaadr(l)
CONS l;
{
   l = mcCdr(l);
   l = mcCar(l);
   l = mcCar(l);
   return l;
}

/* mcCaddr(l) - Returns the CADDR of the list l. */
CONS mcCaddr(l)
CONS l;
{
   l = mcCdr(l);
   l = mcCdr(l);
   l = mcCar(l);
   return l;
}

CONS mcCdar(l)
CONS l;
{
   l = mcCar(l);
   l = mcCdr(l);
   return l;
}

/* mcCadddr(l) - Returns the CADDDR of the list l. */
CONS mcCadddr(l)
CONS l;
{
   l = mcCdr(l);
   l = mcCdr(l);
   l = mcCdr(l);
   l = mcCar(l);
   return l;
}

/* mcCadar(l) - Returns the CADAR of the list l. */
CONS mcCadar(l)
CONS l;
{
   l = mcCar(l);
   l = mcCdr(l);
   l = mcCar(l);
   return l;
}

/* mcCaar(l) - Returns the CAAR of the list l. */
CONS mcCaar(l)
CONS l;
{
   l = mcCar(l);
   l = mcCar(l);
   return l;
}

/* mcCddr(l) - Returns the CDDR of the list l. */
CONS mcCddr(l)
CONS l;
{
   l = mcCdr(l);
   l = mcCdr(l);
   return l;
}

/* mcCdddr(l) - Returns the CDDDR of the list l. */
CONS mcCdddr(l)
CONS l;
{
   l = mcCdr(l);
   l = mcCdr(l);
   l = mcCdr(l);
   return l;
}

/* mcCddddr(l) */
CONS mcCddddr(l)
CONS l;
{
   l = mcCdr(l);
   l = mcCdr(l);
   l = mcCdr(l);
   l = mcCdr(l);
   return l;
}

/* mcCons(head, tail) - Returns a cons node whose car = head and cdr = tail. */
CONS mcCons(head, tail)
CONS head, tail;
{
   ENTER;
   REG(h);
   REG(t);
   REG(res);

   R(h) = head;
   R(t) = tail;

   /* need a cons node */
   R(res) = NewCons( PAIR, 0, 0 );

   /* build the cons node */
   mcGet_Car( R(res) ) = R(h);
   mcGet_Cdr( R(res) ) = R(t);

   MCLEAVE R(res);
}

/* mcSetCar(c, h) - Makes h be the car of c.  This is a list-altering
	function.
*/
CONS mcSetCar(c,h)
CONS c, h;
{
   if ( !mcPair(c) )
	RT_ERROR("Bad arg to mcSetCar.");

   mcGet_Car(c) = h;
   return c;
}

/* mcSetCdr(c, t) - Makes t be the cdr of c.  This is a list altering
	function.
*/
CONS mcSetCdr(c,t)
CONS c, t;
{
   if ( !mcPair(c) )
	RT_ERROR("Bad arg to mcSetCdr.");

   mcGet_Cdr(c) = t;
   return c;
}

/* mcEq(n1, n2) - n1 eq n2 iff:
	(a) n1 and n2 are the same cons node
	(b) n1 and n2 are the same symbol ( 'the eq 'the is true )

	Eq() is undefined for numbers.
*/
CONS mcEq(n1, n2)
CONS n1, n2;
{
   /* same object? */
   if ( n1 == n2 )
	return T;

   /* same type?  symbol? */
   if ( mcKind(n1) == mcKind(n2) && mcKind(n1) == SYMBOL ) {

	/* rely's on fact that symbols are stored ONCE, see symbol.c */
	if ( mcGet_Sym(n1) == mcGet_Sym(n2) )
		return T;
	else return F;
   }

   /* not eq */
   return F;
}

/* mcEqv(n1, n2) - n1 eqv n2 iff:
	(a) n1 and n2 are the same number of the same type
	(b) n1 and n2 are empty strings
	(c) n1 char=? n2
	(b) n1 eq n2
*/
CONS mcEqv(n1, n2)
CONS n1, n2;
{
   /* are they eq already?  this handles symbols */
   if ( mcEq(n1, n2) == T )
	return T;

   /* not eq, better be the same type */
   if ( mcKind(n1) == mcKind(n2) ) {

	/* both integers and the same integer? */
	if ( mcKind(n1) == INT && mcGet_Int(n1) == mcGet_Int(n2) )
		return T;

	/* both floats and the same float? */
	if ( mcKind(n1) == FLOAT && mcGet_Float(n1) == mcGet_Float(n2) )
		return T;

	/* both the empty string? */
	if ( mcKind(n1) == STRING )
		if ( *(mcGet_Str(n1)) == *(mcGet_Str(n2)) && *(mcGet_Str(n1)) == EOS )
			return T;

	/* the same character? */
	if ( mcKind(n1) == CHAR && mcCharE(n1, n2) )
		return T;
   }

   /* not eqv */
   return F;
}

/* ----------------------------------------------------------------------- */
/*                        High Level List Operations			   */
/* ----------------------------------------------------------------------- */

/* mcRev(l) - Given list L, returns L after reversing all of the pointers. */
CONS mcRev(l)
CONS l;
{
   CONS prev, curr, next, ahead;

   for ( prev = NIL, curr = l; curr != NIL ; curr = ahead, prev = next ) {

	next = mcGet_Cdr(curr);
	ahead = ( next != NIL ? mcGet_Cdr(next) : NIL );

	if ( next == NIL ) {
		mcSetCdr(curr, prev);
		prev = curr;
		break;
	}

	mcSetCdr(curr, prev);
	mcSetCdr(next, curr);
   }

   return prev;
}

/* mcAppend(sexpr, lyst) - Appends the sexpr to the end of lyst.  Returns the
	new list.
*/
CONS mcAppend(sexpr, lyst)
CONS sexpr, lyst;
{
   CONS curr;

   /* push sexpr down a level */
   sexpr = mcCons( sexpr, NIL );

   /* appending to NIL => sexpr */
   if ( mcNull(lyst) )
	return sexpr;

   /* can't append to an atom */
   if ( mcAtom(lyst) )
	RT_ERROR("Bad arg to mcAppend!");

   /* appending to list, find end of lyst */
   curr = lyst;
   while ( !mcNull( mcCdr(curr) ) )
	curr = mcCdr(curr);

   /* tack on sexpr */
   curr = mcSetCdr(curr, sexpr);

   return lyst;
}

/* mcEqual(n1, n2) - n1 equal n2 iff they have the same structure. */
CONS mcEqual( n1, n2)
CONS n1, n2;
{
   /* are n1 and n2 eql atoms? */
   if ( mcAtom(n1) || mcAtom(n2) || mcNull(n1) || mcNull(n2) ) {
	/* strings and vectors (not implemented) need to be checked
	 * before eqv? since eqv? doesn't know about them.
	 */
	if ( mcKind(n1) == mcKind(n2) && mcKind(n1) == STRING )
		return ( mcStrE(n1, n2) ? T : F );

	return mcEqv(n1, n2);
   }

   /* (and (equal (car n1) (car n2)) (equal (cdr n1) (cdr n2))) => T */
   if ( mcEqual( mcGet_Car(n1), mcGet_Car(n2) ) != F )
	if ( mcEqual( mcGet_Cdr(n1), mcGet_Cdr(n2) ) != F )
		return T;

   return F;
}

/* mcAssoc(s, l) - Value returned is the first pair in the a-list l such
	that: (equal s (car (Assoc s l)) ) => T
*/
CONS mcAssoc(s, l)
CONS s, l;
{
   CONS head;

   /* traverse top level of l; NOTE: (Pair? #NULL) => #NULL */
   while ( mcPair(l) ) {

	head = mcCar(l);

	/* if (Equal (car head) s ) return l */
	if ( mcEqual( mcCar(head), s) == T )
		return head;

	/* next element */
	l = mcCdr(l);
   }

   /* not found */
   return F;
}

/* mcAssq(s,l) - Assoc that uses EQ? instead of EQUAL? */
CONS mcAssq(s, l)
CONS s, l;
{
   CONS head;

   /* (Pair? #NULL) => #NULL */
   while ( mcPair(l) ) {

	head = mcCar(l);

	if ( mcEq( mcCar(head), s) == T )
		return head;

	l = mcCdr(l);
   }

   return F;
}

/* mcAssv(s,l) - Assoc that uses EQV? instead of EQUAL? */
CONS mcAssv(s, l)
CONS s;
CONS l;
{
   CONS head;

   /* (Pair? #NULL) => #NULL */
   while ( mcPair(l) ) {

	head = mcCar(l);

	if ( mcEqv( mcCar(head), s) == T )
		return head;

	l = mcCdr(l);
   }

   return F;
}

/* mcQAssoc(s, l) - Quick Assoc - should only be used by C code, namely
	opEval.
*/
CONS mcQAssoc(s, l)
CONS s, l;
{
   CONS head, symbol;

   if ( mcNull(l) )
	return NIL;

   /* traverse top level of l; NOTE: (Pair? #NULL) => #NULL */
   while ( mcPair(l) ) {

	/* first binding and first symbol */
	head = mcCar(l);
	symbol = mcCar(head);

	/* same symbol? */
	if ( mcGet_Sym(symbol) == mcGet_Sym(s) )
		return head;

	/* next element */
	l = mcCdr(l);
   }

   /* not found */
   return NIL;
}

/* mcMember(o,l) - Given an object o and list l, returns the first sub-list
	of l whose car is o.  This function uses EQUAL?.
*/
CONS mcMember(o, l)
CONS o,l;
{
   /* (Pair? #NULL) => #NULL */
   while ( mcPair(l) ) {
	if ( mcEqual( mcCar(l), o ) == T )
		return l;

	l = mcCdr(l);
   }

   return F;
}

/* mcMemq(o,l) - Member using EQ? instead of EQUAL? */
CONS mcMemq(o, l)
CONS o,l;
{
   /* (Pair? #NULL) => #NULL */
   while ( mcPair(l) ) {
	if ( mcEq( mcCar(l), o ) == T )
		return l;

	l = mcCdr(l);
   }

   return F;
}

/* mcMemv(o,l) - Member using EQV? instead of EQ? */
CONS mcMemv(o, l)
CONS o,l;
{
   /* (Pair? #NULL) => #NULL */
   while ( mcPair(l) ) {
	if ( mcEqv( mcCar(l), o ) == T )
		return l;

	l = mcCdr(l);
   }

   return F;
}

/* mcLength(l) - Returns top-level length of list l. */
int mcLength(l)
CONS l;
{
   int len;

   for ( len = 0; mcPair(l) ; ++len, l = mcCdr(l) )
	;

   return len;
}

/* mcProcedure(l) - Returns T if the object l is a system defined
	function or a closure.
*/
CONS mcProcedure(l)
CONS l;
{
   if ( mcClosure(l) || mcFunc(l) || mcCont(l) )
	return T;

   return F;
}

/* ----------------------------------------------------------------------- */
/*                              Vector Ops				   */
/* ----------------------------------------------------------------------- */

/* mcArgVector() - Pops args off the value stack and places them into
	a vector.
*/
CONS mcArgVector()
{
   assert(0);
}

/* mcMakeVector(s, o) - Given the size of the array, returns a new vector.  The
	new vector is initialized with object o.
*/
CONS mcMakeVector(s, o)
int s;
CONS o;
{
   CONS vec;

   /* create the vector node */
   vec = NewCons( VECTOR, s, 0 );

   /* initialize it to object o */
   mcVectorFill(vec, o);

   return vec;
}

/* mcLstVector(l) - Convert list l into a vector. */
CONS mcLstVector(l)
CONS l;
{
   int e;
   CONS vec;

   /* make a vector big enough for l, fill it with NILs */
   vec = mcMakeVector( mcLength(l), NIL );

   /* fill the vector in with elements of l */
   e = 0;
   while ( !mcNull(l) ) {
	*mcVect_Ref(vec, e) = mcCar(l);
	l = mcCdr(l);
	++e;
   }

   return vec;
}

/* mcVectorLst(v) - Convert vector v into a list. */
CONS mcVectorLst(v)
CONS v;
{
   int e;

   {
    ENTER;
    REG(l);
    R(l) = NIL;

    for ( e = mcVect_Size(v)-1; e >= 0; --e )
	R(l) = mcCons( *mcVect_Ref(v, e), R(l) );

    MCLEAVE R(l);
   }
}

/* mcVectorCopy(v) - Makes a copy of the vector v. */
CONS mcVectorCopy(v)
CONS v;
{
   ENTER;
   REG(old);
   REG(new);

   /* v can't disappear while R(old) is holding onto it */
   R(old) = v;
   R(new) = mcMakeVector( mcVect_Size(v), NIL );
   {
	int e;

	for ( e = 0; e < mcVect_Size(v) ; ++e )
	   *mcVect_Ref( R(new), e ) = mcTree_Copy( *mcVect_Ref(v, e) );
   }

   MCLEAVE R(new);
}

/* mcVectorFill(v, o) - Fills vector v with object o. */
void mcVectorFill(v, o)
CONS v, o;
{
   int l;

   l = mcVect_Size(v);

   for ( l = mcVect_Size(v)-1; l >= 0; --l )
	*mcVect_Ref(v, l) = o;
}

/* ----------------------------------------------------------------------- */
/*                           Copying Functions				   */
/* ----------------------------------------------------------------------- */

/* mcCopyCons(n1, n2) - Returns a copy of the CONS node. */
CONS mcCopyCons(n)
CONS n;
{
   CONS temp, tmp;

   /* #NULL and #T are special CONS nodes in that only 1 copy ever exists */
   if ( mcNull(n) || n == T || n == F || n == EOF_OBJ )
	return n;

   if ( mcKind(n) == VECTOR ) {
	temp = mcVectorCopy(n);
	return temp;
   }
   else if ( mcKind(n) == BCODES )
	temp = NewCons( BCODES, mcBC_CSize(n), mcBC_CCSize(n) );
   else
	temp = NewCons( mcKind(n), 0, 0 );

   tmp = (CONS) memcpy( (char *)temp, (char *)n, (int)sizeof(CONSNODE) );

   assert( temp == tmp );

   if ( mcCode(n) ) {
	/* copy the byte-code */
	memcpy( (char *)mcBC_Code(temp), (char *)mcBC_Code(n), (int)mcBC_CSize(n) );

	/* copy the constant table */
	memcpy( (char *)mcBC_Const(temp), (char *)mcBC_Const(n), (int)(mcBC_CCSize(n)*sizeof(CONS)) );
   }

   return temp;
}

/* mcTree_Copy(t) - Given list (actually a tree) l, returns a duplicate of
	l.
*/
CONS mcTree_Copy(t)
CONS t;
{
   ENTER;
   REG(copying);
   REG(temp);

   if ( t == NIL ) {
	MCLEAVE NIL;
   }

   R(copying) = t;

   R(temp) = mcCopyCons(t);
   if ( mcPair( R(temp) ) ) {
	mcGet_Car( R(temp) ) = mcTree_Copy( mcGet_Car(t) );
	mcGet_Cdr( R(temp) ) = mcTree_Copy( mcGet_Cdr(t) );
   }

   MCLEAVE R(temp);
}

/* ----------------------------------------------------------------------- */
/*                            String Routines				   */
/* ----------------------------------------------------------------------- */

CONS mcGenSym()
{
   static int num = 0;
   char symbol[15];
   CONS sym;

   /* create a symbol that hasn't been seen before */
   sprintf(symbol, "G%d", num);
   ++num;
   while ( ssIsSymbol(symbol) ) {
	sprintf(symbol, "G%d", num);
	++num;
   }

   sym = NewCons( SYMBOL, 0, 0 );
   mcCpy_Sym(sym, symbol);
   return sym;
}

/* mcSubStr(s, beg, end) - Returns an allocated string which is the portion
	of string s from index beg to index end.
*/
CONS mcSubStr(s, beg, end)
char *s;
int beg, end;
{
   CONS str;

   str = NewCons( STRING, 0, 0 );
   mcCpy_Str( str, ssSubstring( s, beg, end ) );
   return str;
}

/* mcSymStr(s) - Given a CONS node with a symbol in it, returns a CONS
	node with a newly allocated string in it that is a copy of the
	symbol.
*/
CONS mcSymStr(s)
CONS s;
{
   CONS str;

   str = NewCons( STRING, 0, 0 );
   mcCpy_Str( str, ssAddString( mcGet_Sym(s) ) );
   return str;
}

/* mcStrSym(s) - Given a CONS node with a string in it, converts the
	string into a symbol.
*/
CONS mcStrSym(s)
CONS s;
{
   CONS sym;

   sym = NewCons( SYMBOL, 0, 0 );
   mcCpy_Sym( sym, mcGet_Str(s) );
   return sym;
}

/* mcStrApp(s1, s2) - Returns a CONS node with a string which is formed
	from appending s1 and s2.
*/
CONS mcStrApp(s1, s2)
char *s1, *s2;
{
   CONS app;

   app = NewCons( STRING, 0, 0 );
   mcCpy_Str(app, ssAppend(s1, s2));
   return app;
}

/* mcLstStr(l) - Returns the string formed from the list of characters l. */
CONS mcLstStr(l)
CONS l;
{
   CONS ch, str;

   str = NewCons( STRING, 0, 0 );
   mcGet_Str(str) = STRING_SPACE;

   for ( ; !mcNull(l) && Sleft > 0; ++STRING_SPACE, --Sleft ) {
	if ( !mcChar( (ch = mcCar(l)) ) )
		RT_LERROR("LIST->STRING: Atom must be a character: ", ch );

	*STRING_SPACE = mcGet_Char(ch);
	l = mcCdr(l);
   }
   *STRING_SPACE++ = EOS; Sleft --;

   return str;
}

/* mcStrLst(s) - Returns the list of characters which make up string s. */
CONS mcStrLst(s)
char *s;
{
   char *start;

   {
   ENTER;
   REG(lst);
   REG(ch);

   R(lst) = NIL;

   /* find the end of the string */
   for ( start = s; *start != EOS; ++start )
	;

   /* working backwards, copy the characters into a cons node and
    * mcCons() this node into the list of characters.
    */
   for ( --start; start >= s; --start ) {
	R(ch) = NewCons( CHAR, 0, 0 );
	mcCpy_Char( R(ch), *start);

	R(lst) = mcCons( R(ch), R(lst) );
   }

   MCLEAVE( R(lst) );
   }
}

#ifdef NoMemcpy
char *memcpy(d, s, l)
char *d, *s;
int l;
{
#ifdef Bcopy
   return bcopy( s, d, l );
#else
   while ( l > 0 ) {
	*d++ = *s++;
	--l;
   }

   return d;
#endif
}
#endif
