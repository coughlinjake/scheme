/* globals.c - Initialize global variables.

   Version 1
*/

#include "machine.h"

#include "glo.h"
#include "symstr.h"
#include "micro.h"
#include "memory.h"
#include "eval.h"
#include "predefs.h"
#include "ops.h"
#include "forms.h"
#include "preds.h"

/* global variables */
CONS glo_env = (CONS)NULL;	/* Top-Level environment */

/* private proto-types */
static void deffunc( C_CHAR C_PTR X C_INT X C_VOID_F_PTR X C_INT X C_INT );
static void defform( C_CHAR C_PTR X C_INT X C_VOID_F_PTR X C_VOID_F_PTR X C_INT X C_INT );
static void add_predefs( C_VOID );

/* InitGlos() - Initialize global variables. */
void InitGlos()
{
   /* create the environment */
   glo_env = mcMkEnv();

   /* bind EXP_TABLE to '() */
   evDefGlobal( EXP_TABLE, NIL );

   /* add pre-defined functions and forms */
   add_predefs();
}

/* add_predefs() - Adds the pre-defined functions and forms to the
	environment.
*/
static void add_predefs()
{
   /* interpreter directives */
   deffunc("THE-ENVIRONMENT", prEnv, opEnv, 0, 0);
   deffunc("TORTURE", prTorture, opTorture, 0, 0);
   deffunc("GCDEBUG", prGcDebug, opGcDebug, 0, 0);
   deffunc("EVDEBUG", prEvDebug, opEvDebug, 0, 0);
   deffunc("QUIT", prExit, opExit, 0, 0);
   deffunc("EXIT", prExit, opExit, 0, 0);
   deffunc("BYE", prExit, opExit, 0, 0);

   /* primitive list prerations */
   deffunc("CAR", prCar, opCar, 1, 1);
   deffunc("CDR", prCdr, opCdr, 1, 1);
   deffunc("CONS", prCons, opCons, 2, 2);
   deffunc("SET-CAR!", prSetCar, opSetCar, 2, 2);
   deffunc("SET-CDR!", prSetCdr, opSetCdr, 2, 2);

   /* predicates */
   deffunc("NULL?", prNull, opNull, 1, 1);
   deffunc("ATOM?", prAtom, opAtom, 1, 1);
   deffunc("PAIR?", prPair, opPair, 1, 1);
   deffunc("SYMBOL?", prSymbol, opSymbol, 1, 1);
   deffunc("NUMBER?", prNumber, opNumber, 1, 1);
   deffunc("INTEGER?", prInteger, opInteger, 1, 1);
   deffunc("FLOAT?", prFloat, opFloat, 1, 1);
   deffunc("ZERO?", prZero, opZero, 1, 1);

   /* equality tests */
   deffunc("EQ?", prEq, opEq, 2, 2);
   deffunc("EQV?", prEqv, opEqv, 2, 2);
   deffunc("EQUAL?", prEqual, opEqual, 2, 2);

   /* math operations */
   deffunc("+", prPlus, opPlus, 0, -1);
   deffunc("-", prMinus, opMinus, 0, -1);
   deffunc("*", prMult, opMult, 0, -1);
   deffunc("/", prDiv, opDiv, 0, -1);
   deffunc("ABS", prAbs, opAbs, 1, 1);
   deffunc("<", prLT, opLT, 2, 2);
   deffunc(">", prGT, opGT, 2, 2);
   deffunc("<=", prLTE, opLTE, 2, 2);
   deffunc(">=", prGTE, opGTE, 2, 2);
   deffunc("=", prE, opE, 2, 2);
   deffunc("<>", prNE, opNE, 2, 2);

   /* special forms */
   defform("LAMBDA", prLambda, opLambda, opNoOp, 2, -1);
   defform("DEFINE", prDefine, opDefine, bcDefine, 2, -1);
   defform("SET!", prSet, opSet, bcSet, 2, 2);
   defform("IF", prIf, opIf, opNoOp, 2, 3);
   defform("QUOTE", prQuote, opQuote, opNoOp, 1, 1);
   defform("BEGIN", prBegin, opBegin, opNoOp, 0, -1);
   defform("OR", prOr, opOr, opNoOp, 0, -1);
   defform("AND", prAnd, opAnd, opNoOp, 0, -1);
   defform("MACRO", prMacro, opMacro, bcMacro, 2, 2);

   /* higher-level list functions */
   deffunc("ASSOC", prAssoc, opAssoc, 2, 2);
   deffunc("ASSQ", prAssq, opAssq, 2, 2);
   deffunc("ASSV", prAssv, opAssv, 2, 2);
   deffunc("MEMBER", prMember, opMember, 2, 2);
   deffunc("MEMQ", prMemq, opMemq, 2, 2);
   deffunc("MEMV", prMemv, opMemv, 2, 2);
   deffunc("LIST", prList, opList, 1, -1);
   deffunc("LENGTH", prLength, opLength, 1, 1);
   deffunc("APPEND", prAppend, opAppend, 2, -1);
   deffunc("REVERSE", prRev, opRev, 1, 1);
   deffunc("TREE-COPY", prTreeCopy, opTree_Copy, 1, 1);

   /* Misc */
   deffunc("PROCEDURE?", prProcedure, opProcedure, 1, 1);
   deffunc("EVAL", prEval, opEval, 1, 2);
   deffunc("APPLY", prApply, opApply, 2, 2);
   deffunc("CALL/CC", prCallCC, opCallCC, 1, 1);
   deffunc("CALL-WITH-CURRENT-CONTINUATION", prCallCC, opCallCC, 1, 1);

   /* environment routines */
   deffunc("DUMP-ENVIRONMENT", prDumpEnv, opDumpEnv, 1, 1);
   deffunc("RESTORE-ENVIRONMENT", prRestEnv, opRestEnv, 1, 1);

   /* Boolean functions */
   deffunc("NOT", prNot, opNot, 1, 1);

   /* Character functions */
   deffunc("CHAR?", prChar, opChar, 1, 1);
   deffunc("CHAR=?", prCharE, opCharE, 2, 2);
   deffunc("CHAR<?", prCharL, opCharL, 2, 2);
   deffunc("CHAR>?", prCharG, opCharG, 2, 2);
   deffunc("CHAR<=?", prCharLE, opCharLE, 2, 2);
   deffunc("CHAR>=?", prCharGE, opCharGE, 2, 2);
   deffunc("CHAR->INTEGER", prCharInt, opCharInt, 1, 1);
   deffunc("INTEGER->CHAR", prIntChar, opIntChar, 1, 1);

   /* String functions */
   deffunc("STRING?", prString, opString,  1, 1);
   deffunc("STRING-LENGTH", prStrLen, opStrLen, 1, 1);
   deffunc("STRING-REF", prStrRef, opStrRef, 2, 2);
   deffunc("STRING=?", prStrE, opStrE, 2, 2);
   deffunc("STRING<?", prStrL, opStrL, 2, 2);
   deffunc("STRING>?", prStrG, opStrG, 2, 2);
   deffunc("STRING<=?", prStrLE, opStrLE, 2, 2);
   deffunc("STRING>=?", prStrGE, opStrGE, 2, 2);
   deffunc("SUBSTRING", prSubStr, opSubStr, 3, 3);
   deffunc("STRING->LIST", prStrLst, opStrLst, 1, 1);
   deffunc("LIST->STRING", prLstStr, opLstStr, 1, 1);
   deffunc("SYMBOL->STRING", prSymStr, opSymStr, 1, 1);
   deffunc("STRING->SYMBOL", prStrSym, opStrSym, 1, 1);
   deffunc("STRING-APPEND", prStrApp, opStrApp, 2, 2);

   /* vector routines */
   deffunc("VECTOR?", prVector, opVector, 1, 1);
   deffunc("VECTOR", prArgVector, opArgVector, 0, -1);
   deffunc("MAKE-VECTOR", prMakeVector, opMakeVector, 2, 2);
   deffunc("VECTOR-LENGTH", prVectLength, opVectLength, 1, 1);
   deffunc("VECTOR-REF", prVectRef, opVectRef, 2, 2);
   deffunc("VECTOR-SET!", prVectSet, opVectSet, 3, 3);
   deffunc("VECTOR-COPY", prVectCopy, opVectCopy, 1, 1);
   deffunc("VECTOR-FILL!", prVectFill, opVectFill, 2, 2);
   deffunc("VECTOR->LIST", prVectLst, opVectLst, 1, 1);
   deffunc("LIST->VECTOR", prLstVect, opLstVect, 1, 1);

   /* I/O routines */
   deffunc("READ", prRead, opRead, 0, 1);
   deffunc("WRITE", prWrite, opWrite, 1, 2);
   deffunc("BOOLEAN?", prBoolean, opBoolean, 1, 1);
   deffunc("READ-CHAR", prReadChar, opReadChar, 0, 1);
   deffunc("WRITE-CHAR", prWriteChar, opWriteChar, 1, 2);
   deffunc("EOF-OBJECT?", prEofObj, opEofObj, 1, 1);
   deffunc("DISPLAY", prDisplay, opDisplay, 1, 2);
   deffunc("NEWLINE", prNewLine, opNewLine, 0, 1);
   deffunc("INPUT-PORT?", prInPort, opInPort, 1, 1);
   deffunc("OUTPUT-PORT?", prOutPort, opOutPort, 1, 1);
   deffunc("CURRENT-INPUT-PORT", prCurrIn, opCurrIn, 0, 0);
   deffunc("CURRENT-OUTPUT-PORT", prCurrOut, opCurrOut, 0, 0);
   deffunc("OPEN-INPUT-FILE", prOpenInFile, opOpenInFile, 1, 1);
   deffunc("OPEN-OUTPUT-FILE", prOpenOutFile, opOpenOutFile, 1, 1);
   deffunc("CLOSE-FILE", prClose, opClose, 1, 1);
   deffunc("LOAD", prLoad, opLoad, 1, 1);

   deffunc("ERROR", prError, opError, 0, -1);
   deffunc("GENSYM", prGenSym, opGenSym, 0, 0);
   deffunc("*COMPILE*", prCompile, opCompile, 1, 1);
   deffunc("CHDIR", prChdir, opChdir, 1, 1);
}

/* ----------------------------------------------------------------------- */
/*                            Local Routines				   */
/* ----------------------------------------------------------------------- */

/* deffunc(name, pr, op, ra, aa) - Creates the binding for the system
	function.
	* pr is the predefined #
	* op is the interpreter	operation,
	* ra is the # required args
	* aa is the allowed args
*/
static void deffunc(name, pr, op, ra, aa)
char *name;
int pr;
void (*op)( C_VOID );
int ra, aa;
{
   ENTER;
   REG(symbol);
   REG(func);

   R(symbol) = NewCons( SYMBOL, 0, 0 );
   mcCpy_Sym( R(symbol), name );

   /* create the primitive function definition node */
   R(func) = NewCons( CFUNC, 0, 0 );
   mcPrim_Name( R(func) ) = mcGet_Sym( R(symbol) );
   mcPrim_PR( R(func) ) = pr;
   mcPrim_RA( R(func) ) = ra;
   mcPrim_AA( R(func) ) = aa;
   mcPrim_Op( R(func) ) = op;

   evAddFunc( pr, op );

   evDefGlobal( R(symbol), R(func) );

   MCLEAVE;
}

/* defform(name, pr, op, bc, ra, aa) - Creates the binding for the system
	special form.
	* pr is the pr number,
	* op is the interpreter	operation,
	* bc is the byte-code interpreter operation,
	* rs is the resume operation,
	* ra is the # required args
	* aa is the allowed args
*/
static void defform(name, pr, op, bc, ra, aa)
char *name;
int pr;
void (*op)( C_VOID );
void (*bc)( C_VOID );
int ra, aa;
{
   ENTER;
   REG(symbol);
   REG(form);

   R(symbol) = NewCons( SYMBOL, 0, 0 );
   mcCpy_Sym( R(symbol), name );

   /* create the form's definition node */
   R(form) = NewCons( CFORM, 0, 0 );
   mcPrim_Name( R(form) ) = mcGet_Sym( R(symbol) );
   mcPrim_PR( R(form) ) = pr;
   mcPrim_RA( R(form) ) = ra;
   mcPrim_AA( R(form) ) = aa;
   mcPrim_Op( R(form) ) = op;

   /* let the compiler know about this form */
   evAddFunc( pr, bc );

   evDefGlobal( R(symbol), R(form) );

   MCLEAVE;
}
