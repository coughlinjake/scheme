/* forms.c - the higher level, Scheme special forms.

   Version 1

   NOTES:
	- Notation: DP means dotted-pair.

	- See notes in eval.c, ops.c.

	- The global environment contains a defn for *EXPANSION-TABLE*.  This
	is bound to an a-list which binds pairs of the form:
			 ( MACRO-NAME . EXPANDER-FUNCTION ).

		* At startup, *EXPANSION-TABLE* is bound to '().

		* This scheme allows user's to have variables sharing the
		same symbol as a macro.  The system only uses it as a macro
		when defined by Scheme's syntax rule: (function arg1 ...).
		After all, macro's are syntax, and are NOT really bindings
		like functions.

		* EXP_TABLE is the symbol CONS node ``*EXPANSION-TABLE*''.
		We need this because evAccGlobal() requires the symbol to
		be in a CONS node.

	- Kent Dybvig's code for extend-syntax will make macro bindings.
	After all, extend-syntax does create macros -- it's just that
	extend-syntax generates the expansion function for you.
*/

#include "machine.h"

#include STRING_H

#include "glo.h"
#include "symstr.h"
#include "micro.h"
#include "memory.h"
#include "error.h"
#include "predefs.h"
#include "eval.h"
#include "forms.h"

/* local prototypes */
static void fmThrowAwayVal( C_VOID );

/* fmThrowAwayVal() - Pops all exps off the value stack to the MARK. */
static void fmThrowAwayVal()
{
   CONS curr;

   while ( mcHaveVals() ) {
	curr = mcPopVal();
	if ( curr == MARK )
		break;
   }
}

/* -----------------------------------------------------------------------
                          Compiled Special Forms

   * These operations are called by the byte-code interpreter.  The special
   form's args were already evaluated since the compiler generated code to
   evaluate the arguments BEFORE invoking the form.  These ops just need
   to pop their args and perform their operation.

   * Special forms that are interpreted are prefixed with "op".
   ----------------------------------------------------------------------- */

void bcMacro()
{
   assert(0);
}

/* (SET! symbol value) */
void bcSet()
{
   ENTER;
   REG(symbol);
   REG(value);
   REG(binding);

   R(value) = mcPopVal();
   R(symbol) = mcPopVal();

   /* SET! a local binding in the nested_env BEFORE trying to SET! a
    * global binding.
    */
   R(binding) = evAccNested( R(symbol), glo_env );
   if ( !mcNull( R(binding) ) ) {
	mcSetCdr( R(binding), R(value) );
   }
   /* no local binding, see if there is a global binding */
   else if ( evAccGlobal( R(symbol), glo_env ) != NULL ) {
	evDefGlobal( R(symbol), R(value) );
   }
   else {
	/* no binding at all => error */
	RT_LERROR("SET!: Symbol undefined: ", R(symbol) );
   }

   OPLEAVE( R(symbol) );
}

/* (DEFINE symbol value) */
void bcDefine()
{
   CONS val, sym;

   val = mcPopVal();
   sym = mcPopVal();

   if ( !mcSymbol(sym) ) {
	RT_LERROR("DEFINE: Can't bind to non-symbol: ", sym);
   }

   if ( evAccGlobal(sym, glo_env) != NULL ) {
	RT_LERROR("DEFINE: Symbol already defined: ", sym);
   }

   evDefGlobal( sym, val );

   mcPushVal(sym);
}

/* ----------------------------------------------------------------------- */
/*                       Interpreted Special Forms			   */
/* ----------------------------------------------------------------------- */

/* opQuote(args) */
void opQuote()
{
   mcPushVal( mcPopExpr() );

   /* pop CALL off expr stack */
   (void)mcPopExpr();
}

/* opLambda() - Returns a closure of the lambda function. */
void opLambda()
{
   ENTER;
   REG(close);
   REG(parms);
   REG(body);

   /* gather the parm-list and body off the expression stack */
   R(close) = evGatherExpr();
   R(parms) = mcCar( R(close) );
   R(body) = mcCdr( R(close) );

   /* build the closure */
   R(close) = NewCons( CLOSURE, 0, 0 );
   mcCl_Parms(R(close)) = R(parms);
   mcCl_Body(R(close)) = R(body);
   mcCl_Env(R(close)) = mcGet_Nested(glo_env);

   /* return the closure */
   OPLEAVE( R(close) );
}

/* ----------------------------------------------------------------------- */
/*                           Binding Forms				   */
/* ----------------------------------------------------------------------- */

/* opDefine() - (DEFINE ATOM EXP)  Evaluate EXP and bind it's value to
	ATOM.
	NOTE: No need to worry about tail recursion.
*/
void opDefine()
{
   ENTER;
   REG(symbol);
   REG(exp);

   /* gather args */
   R(exp) = evGatherExpr();

   R(symbol) = mcCar( R(exp) );

   /* special form of define:
	(define (name parms) body) => (name (lambda (parms) body)) */
   if ( !mcAtom( R(symbol) ) ) {

	REG(close);

	/* build the closure */
	R(close) = NewCons( CLOSURE, 0, 0 );
	mcCl_Parms(R(close)) = mcCdr(R(symbol));
	mcCl_Body(R(close)) = mcCdr( R(exp) );
	mcCl_Env(R(close)) = mcGet_Nested(glo_env);

	/* strip off function name */
	R(symbol) = mcCar( R(symbol) );

	/* is there an old binding? */
	evDefGlobal( R(symbol), R(close) );

	OPLEAVE( R(symbol) );
   }

   if ( !mcSymbol( R(symbol) ) )
	RT_LERROR("DEFINE: Can't bind to non-symbol: ", R(symbol) );

   /* save symbol on value stack */
   mcPushVal( R(symbol) );

   /* push MARK to separate symbol from value of exp */
   mcPushVal( MARK );

   /* evaluate exp then resume 'DEFINE' */
   mcPushExpr( evMkResume(prDefine) );
   mcPushExpr( mcCadr( R(exp) ) );

   OPVOIDLEAVE;
}

/* opResDefine() - Resume defining something. */
void opResDefine()
{
   ENTER;
   REG(symbol);
   REG(value);

   /* pop value, MARK, and symbol */
   R(value) = mcPopVal();
   (void)mcPopVal();
   R(symbol) = mcPopVal();

   if ( evAccGlobal( R(symbol), glo_env ) != NULL )
	RT_LERROR("Symbol already defined: ", R(symbol) );

   evDefGlobal( R(symbol), R(value) );

   OPLEAVE( R(symbol) );
}

/* opSet() - (SET ATOM EXP)  Evaluate EXP and bind it's value to
	ATOM.  This is the interpreter operation for SET!.

	NOTE: No need to worry about tail recursion.
*/
void opSet()
{
   ENTER;
   REG(symbol);
   REG(exp);

   /* same trick as in 'DEFINE' */
   R(exp) = mcPopExpr();
   R(symbol) = mcPopExpr();
   (void) mcPopExpr();

   if ( !mcSymbol( R(symbol) ) )
	RT_LERROR("SET!: Can't bind to non-symbol: ", R(symbol) );

   mcPushVal( R(symbol) );
   mcPushVal( MARK );

   /* setup to evaluate second exp */
   mcPushExpr( evMkResume(prSet) );
   mcPushExpr( R(exp) );

   OPVOIDLEAVE;
}

/* opResSet() - Resume "setting" something. */
void opResSet()
{
   ENTER;
   REG(symbol);
   REG(value);
   REG(binding);

   R(value) = mcPopVal();
   (void) mcPopVal();
   R(symbol) = mcPopVal();

   /* SET! a local binding in the nested_env BEFORE trying to SET! a
    * global binding.
    */
   R(binding) = evAccNested( R(symbol), glo_env );
   if ( !mcNull( R(binding) ) ) {
	mcSetCdr( R(binding), R(value) );
   }
   /* no local binding, see if there is a global binding */
   else if ( evAccGlobal( R(symbol), glo_env ) != NULL ) {
	evDefGlobal( R(symbol), R(value) );
   }
   else {
	/* no binding at all => error */
	RT_LERROR("SET!: Symbol undefined: ", R(symbol) );
   }

   OPLEAVE( R(symbol) );
}

/* ----------------------------------------------------------------------- */
/*                      Evaluation Control Forms			   */
/* ----------------------------------------------------------------------- */

/* opBegin() - Takes a list of expressions, and evaluates each
	expression.  Returns the value of the LAST expression.
	NOTE:  Have to be careful about tail recursion here.
*/
void opBegin()
{
   ENTER;
   REG(exp);

   /* pop args off expr stack and place on val stack */
   R(exp) = mcPopExpr();
   if ( R(exp) == CALL ) {
	/* (BEGIN) => NIL though it could be flagged as an error */
	OPLEAVE( NIL );
   } else mcPushVal( MARK );

   while ( R(exp) != CALL ) {
   	mcPushVal( R(exp) );
	R(exp) = mcPopExpr();
   }

   /* push a dummy value on the value stack so we can just call opResBegin() */
   mcPushVal( NIL );

   opResBegin( evMkResume(prBegin) );
   OPVOIDLEAVE;
}

/* opResBegin() - Resume "beginning" */
void opResBegin(res)
CONS res;
{
   CONS exp;

   /* throw away value of previous expression */
   mcPopVal();

   /* get next expression to evaluate */
   exp = mcPopVal();

   /* if this is the last exp, pop the marker.  otherwise push the resume.
    * this handles tail-recursion since the last exp to evaluate won't
    * have a resume underneath it.
    */
   if ( mcValStackTop() == MARK )
	mcPopVal();
   else mcPushExpr( res );

   mcPushExpr( exp );
}

/* opIf - (IF condition consequence {alternate}) - if the evalutation of
	CONDITION yields non-nil, return the evalutation of CONSEQUENCE.
	Otherwise, return the evaluation of ALTERNATE.
	NOTE: No need to worry about tail recursion.
*/
void opIf()
{
   ENTER;
   REG(cond);
   REG(cons);
   REG(alt);

   /* pop parameters */
   R(alt) = mcPopExpr();
   R(cons) = mcPopExpr();
   R(cond) = mcPopExpr();

   if ( R(cond) == CALL ) {
	/* no alternative was specified shift values down into
	 * their correct variables.
	 */
	R(cond) = R(cons);
	R(cons) = R(alt);
	R(alt) = CALL;
   }
   else
	/* pop CALL */
	mcPopExpr();

   /* save consequense and alternate on value stack */
   mcPushVal( R(alt) );
   mcPushVal( R(cons) );
   mcPushVal( MARK );

   mcPushExpr( evMkResume(prIf) );
   mcPushExpr( R(cond) );

   OPVOIDLEAVE;
}

/* opResIf() - Resume 'IF' */
void opResIf()
{
   ENTER;
   REG(cons);
   REG(alt);
   REG(value);

   /* pop value of exp, MARK, cons, alt */
   R(value) = mcPopVal();
   (void) mcPopVal();
   R(cons) = mcPopVal();
   R(alt) = mcPopVal();

   EV_DEBUG("\nBACK IN opResIf, VALUE = ", R(value) );
   EV_DEBUG("\n               , CONS  = ", R(cons) );
   EV_DEBUG("\n               , ALT   = ", R(alt) );

   /* if the result is '() or #f then evaluate alt */
   if ( mcNull( R(value) ) || R(value) == F ) {
	/* if there is no alternate, value of 'IF' is value of condition */
	if ( R(alt) == CALL )
		mcPushVal( R(value) );
	/* otherwise, evaluate alternate */
	else mcPushExpr( R(alt) );
	OPVOIDLEAVE;
   }

   /* evaluate the consequence */
   mcPushExpr( R(cons) );
   OPVOIDLEAVE;
}

/* opOr - (OR exp exp ...) - Return the value of the first non-nil exp (short-
	circuit evaluation).  If all exps evaluate to NIL, return NIL.
	NOTE: Have to watch tail recursion here.
*/
void opOr()
{
   ENTER;
   REG(exp);

   /* pop args off expr stack and place on val stack */
   R(exp) = mcPopExpr();
   if ( R(exp) == CALL ) {
	/* (OR) => #f */
	OPLEAVE( F );
   } else mcPushVal( MARK );

   while ( R(exp) != CALL ) {
	mcPushVal( R(exp) );
	R(exp) = mcPopExpr();
   }

   /* push dummy value on val stack so we can just call opResOr() */
   mcPushVal( NIL );

   opResOr( evMkResume(prOr) );
   OPVOIDLEAVE;
}

/* opResOr() - Resume "oring" */
void opResOr(res)
CONS res;
{
   CONS result, exp;

   result = mcPopVal();
   if ( !mcNull(result) && result != F ) {
	/* throw away other exps */
	fmThrowAwayVal();

	/* done oring */
	mcPushVal( result );
	return;
   }

   /* get next exp to evaluate */
   exp = mcPopVal();

   /* if this is the last exp, pop the marker.  otherwise push the resume.
    * this handles tail-recursion since the last exp to evaluate won't
    * have a resume underneath it.
    */
   if ( mcValStackTop() == MARK )
	mcPopVal();
   else mcPushExpr( res );

   mcPushExpr( exp );
}

/* opAnd - (AND exp exp ...) - Return false as soon as an exp evaluates to
	NIL (short-circuit evaluation).  Return the value of the last exp
	if none eval to #F.
	NOTE: Have to watch tail recursion here.
*/
void opAnd()
{
   ENTER;
   REG(exp);

   /* pop args off expr stack and place on val stack */
   R(exp) = mcPopExpr();
   if ( R(exp) == CALL ) {
	/* (AND) => #t */
	OPLEAVE( T );
   } else mcPushVal( MARK );

   while ( R(exp) != CALL ) {
	mcPushVal( R(exp) );
	R(exp) = mcPopExpr();
   }

   /* push dummy value on val stack so we can just call opResAnd() */
   mcPushVal( T );

   opResAnd( evMkResume(prAnd) );
   OPVOIDLEAVE;
}

/* opResAnd() - Resume "anding" */
void opResAnd(res)
CONS res;
{
   CONS result, exp;

   result = mcPopVal();
   if ( mcNull(result) || result == F ) {
	/* throw away other exps */
	fmThrowAwayVal();

	/* done anding */
	mcPushVal( result );
	return;
   }

   /* get next exp to evaluate */
   exp = mcPopVal();

   /* if this is the last exp, pop the marker.  otherwise push the resume.
    * this handles tail-recursion since the last exp to evaluate won't
    * have a resume underneath it.
    */
   if ( mcValStackTop() == MARK )
	mcPopVal();
   else mcPushExpr( res );

   mcPushExpr( exp );
}

/* opMacro() - Defines a macro. */
void opMacro()
{
   ENTER;
   REG(func);
   REG(symbol);

   /* pop args and CALL */
   R(func) = mcPopExpr();
   R(symbol) = mcPopExpr();
   (void) mcPopExpr();

   if ( !mcSymbol( R(symbol) ) )
	RT_LERROR("MACRO: Can't make macro of non-symbol: ", R(symbol) );

   /* save symbol on val stack */
   mcPushVal( R(symbol) );

   /* setup to evaluate func */
   mcPushExpr( evMkResume(prMacro) );
   mcPushExpr( R(func) );

   OPVOIDLEAVE;
}

/* opResMacro() - Resume defining a macro. */
void opResMacro()
{
   ENTER;
   REG(name);		/* macro name */
   REG(expand);		/* expander function */
   REG(binding);	/* ( name . expander ) */
   REG(etbl);		/* expansion table */

   R(expand) = mcPopVal();
   R(name) = mcPopVal();

   /* get the expansion table from the environment */
   R(etbl) = evAccGlobal( EXP_TABLE, glo_env );

   /* is there an old macro binding? */
   R(binding) = mcQAssoc( R(name), R(etbl) );
   if ( !mcNull( R(binding) ) )
	/* replace old macro binding */
	(void) mcSetCdr( R(binding), R(expand) );

   else {
	/* build a new binding, tack on front of expansion table
	 * and rebind the expansion table.
	 */
	R(binding) = mcCons( R(name), R(expand) );
	R(etbl) = mcCons( R(binding), R(etbl) );
	evDefGlobal( EXP_TABLE, R(etbl) );
   }

   OPLEAVE( R(name) );
}
