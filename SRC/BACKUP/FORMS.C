/* forms.c - the higher level, Scheme special forms.

   Version 1

   NOTES:
	- Notation: DP means dotted-pair.

	- See notes in eval.c, ops.c.

	- The global environment contains a defn for *EXPANSION-TABLE*.
		* glo_env = ( bindings (*EXPANSION-TABLE* . ALIST) bindings )
			where bindings are function and variable bindings,
			      *EXPANSION-TABLE* is a symbol

		* *EXPANSION-TABLE*'s ALIST contains pairs of the form:
			 ( MACRO-NAME . EXPANDER-FUNCTION ).

		* I like this scheme because it means that user's can have
		variables sharing the same symbol as a macro.  The system
		only uses it as a macro when defined by Scheme's syntax
		rule: (function arg1 ...).  After all, macro's are syntax,
		and are NOT really bindings like functions.

		* exp_table is a pointer to this ALIST (so i don't need to
		mcQAssoc() for the table everytime someone defines a macro
		or does an extend-syntax).

		* Initially, the ALIST is () so the 1st macro defn must
		modify the environment so that the environment is
		consistent with exp_table.  Keeping the table in the
		environment allows Scheme code to play with it.

		* Also, since exp_table is a pointer into the environment,
		there's no need to worry about it being GCed -- the env is
		always marked.

	- I modified Kent Dybvig's code for extend-syntax to make macro
	bindings.  After all, extend-syntax does create macros -- it's just
	that extend-syntax generates the expansion function for you whereas
	you explicitly write a macro's expander function.

	- Added define-form to allow users to add their own special forms.
	Special forms are dynamic scope -- you evaluate args that need to
	be evaluated in the caller's environment instead of in the form's
	environment.  Defn of form is:

		(*FORM* parms body)
*/

#include "machine.h"

#ifdef StringH
#include <string.h>
#endif

#ifdef StringsH
#include <strings.h>
#endif

#include "glo.h"
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
   forms args were already evaluated since the compiler generated code to
   evaluate the arguments BEFORE invoking the form.  These ops just need
   to pop their args and perform their operation.

   * Special forms that are interpreted are in forms.c
   ----------------------------------------------------------------------- */

void bcMacro()
{
   assert(0);
}

/* (SET! symbol value) */
void bcSet()
{
   CONS val, sym;

   val = mcPopVal();
   sym = mcPopVal();

   sym = mcQAssoc(sym, glo_env);
   if ( mcNull(sym) ) {
	RT_LERROR("SET!: Symbol not defined: ", sym);
   }

   mcSetCdr(sym, val);
   mcPushVal(sym);
}

/* (DEFINE symbol value) */
void bcDefine()
{
   CONS val, sym;

   val = mcPopVal();
   sym = mcPopVal();

   if ( !mcNull( mcQAssoc(sym, glo_env) ) ) {
	RT_LERROR("DEFINE: Symbol already defined: ", sym);
   }

   val = mcCons(sym, val);
   glo_env = mcAppend(val, glo_env);

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

#ifdef WRONG
   /* reverse body to save time when pushing the body on the expression
    * stack as args to opBegin().
    */
   R(body) = mcRev( R(body) );
#endif

   /* build the closure */
   R(close) = NewCons( CLOSURE, 0, 0 );
   mcCl_Parms(R(close)) = R(parms);
   mcCl_Body(R(close)) = R(body);
   mcCl_Env(R(close)) = glo_env;

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
   REG(resume);
   REG(symbol);
   REG(exp);

   /* gather args */
   R(exp) = evGatherExpr();

   R(symbol) = mcCar( R(exp) );

   /* special form of define:
	(define (name parms) body) => (name (lambda (parms) body)) */
   if ( !mcAtom( R(symbol) ) ) {

	REG(close);
	REG(binding);

	/* build the closure */
	R(close) = NewCons( CLOSURE, 0, 0 );
	mcCl_Parms(R(close)) = mcCdr(R(symbol));
	mcCl_Body(R(close)) = mcCdr( R(exp) );
	mcCl_Env(R(close)) = glo_env;

	/* strip off function name */
	R(symbol) = mcCar( R(symbol) );

	/* is there an old binding? */
	R(binding) = mcQAssoc( R(symbol), glo_env );
	if ( !mcNull( R(binding) ) )
		/* replace old binding */
		(void) mcSetCdr( R(binding), R(close) );
	else {
		/* build a new binding */
		R(binding) = mcCons( R(symbol), R(close) );
		glo_env = mcAppend( R(binding), glo_env );
	}

	OPLEAVE( R(symbol) );
   }

   if ( !mcSymbol( R(symbol) ) )
	RT_LERROR("DEFINE: Can't bind to non-symbol: ", R(symbol) );

   /* save symbol on value stack */
   mcPushVal( R(symbol) );

   /* push MARK to separate symbol from value of exp */
   mcPushVal( MARK );

   /* setup to evaluate exp */
   R(resume) = mcCons( mcIntToCons(prDefine), R(resume) );
   R(resume) = mcCons( RESUME, R(resume) );

   /* evaluate exp then resume 'DEFINE' */
   mcPushExpr( R(resume) );
   mcPushExpr( mcCadr( R(exp) ) );

   OPVOIDLEAVE;
}

/* opResDefine() - Resume defining something. */
void opResDefine(res)
CONS res;
{
   ENTER;
   REG(symbol);
   REG(value);
   REG(binding);

   /* pop value, MARK, and symbol */
   R(value) = mcPopVal();
   (void)mcPopVal();
   R(symbol) = mcPopVal();

   /* make sure this is the first definition */
   R(binding) = mcQAssoc( R(symbol), glo_env );
   if ( !mcNull(R(binding)) )
	RT_LERROR("Symbol already defined: ", R(symbol) );

   /* make binding and add to global environment */
   R(binding) = mcCons( R(symbol), R(value) );
   glo_env = mcAppend( R(binding), glo_env );

   OPLEAVE( R(symbol) );
}

/* opSet() - (SET ATOM EXP)  Evaluate EXP and bind it's value to
	ATOM.
	NOTE: No need to worry about tail recursion.
*/
void opSet()
{
   ENTER;
   REG(resume);
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
   R(resume) = mcCons( mcIntToCons(prSet), R(resume) );
   R(resume) = mcCons( RESUME, R(resume) );
   mcPushExpr( R(resume) );
   mcPushExpr( R(exp) );

   OPVOIDLEAVE;
}

/* opResSet() - Resume "setting" something. */
void opResSet(res)
CONS res;
{
   ENTER;
   REG(symbol);
   REG(value);
   REG(binding);

   R(value) = mcPopVal();
   (void) mcPopVal();
   R(symbol) = mcPopVal();

   R(binding) = mcQAssoc( R(symbol), glo_env );

   /* make sure this isn't the first definition */
   if ( mcNull( R(binding) ) )
	RT_LERROR("SET!: Undefined symbol: ", R(symbol) );

   /* change the binding */
   mcSetCdr( R(binding), R(value) );

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
   REG(resume);
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

   /* build a resume */
   R(resume) = mcCons( mcIntToCons(prBegin), NIL );
   R(resume) = mcCons( RESUME, R(resume) );

   opResBegin( R(resume) );
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
   REG(resume);
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

   R(resume) = mcCons( mcIntToCons(prIf), R(resume) );
   R(resume) = mcCons( RESUME, R(resume) );

   mcPushExpr( R(resume) );
   mcPushExpr( R(cond) );

   OPVOIDLEAVE;
}

/* opResIf() - Resume 'IF' */
void opResIf(res)
CONS res;
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
   REG(resume);
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

   /* build a resume */
   R(resume) = mcCons( mcIntToCons(prOr), NIL );
   R(resume) = mcCons( RESUME, R(resume) );

   opResOr( R(resume) );
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
   REG(resume);
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

   /* build a resume */
   R(resume) = mcCons( mcIntToCons(prAnd), NIL );
   R(resume) = mcCons( RESUME, R(resume) );

   opResAnd( R(resume) );
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
   REG(resume);

   /* pop args and CALL */
   R(func) = mcPopExpr();
   R(symbol) = mcPopExpr();
   (void) mcPopExpr();

   if ( !mcSymbol( R(symbol) ) )
	RT_LERROR("MACRO: Can't make macro of non-symbol: ", R(symbol) );

   /* save symbol on val stack */
   mcPushVal( R(symbol) );

   /* build resume */
   R(resume) = mcCons( mcIntToCons(prMacro), NIL );
   R(resume) = mcCons( RESUME, R(resume) );

   /* setup to evaluate func */
   mcPushExpr( R(resume) );
   mcPushExpr( R(func) );

   OPVOIDLEAVE;
}

/* opResMacro(args) - Resume defining a macro. */
void opResMacro(res)
CONS res;
{
   ENTER;
   REG(name);
   REG(expand);
   REG(binding);

   R(expand) = mcPopVal();
   R(name) = mcPopVal();

   /* is there an old binding? */
   R(binding) = mcQAssoc( R(name), exp_table );
   if ( !mcNull( R(binding) ) )
	/* replace old macro binding */
	(void) mcSetCdr( R(binding), R(expand) );

   else {
	/* build a new binding */
	R(binding) = mcCons( R(name), R(expand) );
	if ( exp_table == NIL ) {
		/* got to keep the environment consistent with the
		 * quick access pointer
		 */
		exp_table = mcAppend( R(binding), exp_table );
		R(binding) = mcQAssoc( EXPANSION, glo_env );
		mcSetCdr( R(binding), exp_table );
	} else exp_table = mcAppend( R(binding), exp_table );
   }

   OPLEAVE( R(name) );
}
