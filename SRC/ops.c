/* ops.c - Scheme operations.
	written by Jason Coughlin

   Version 1

	* Args are dangerous since nothing is holding onto them.  Therefore,
	each op should pop it's args into registers.  However, there is
	overhead associated with registers so not every op does this.  When
	in doubt, use registers.

	* Operations take their arguments off of the value stack,
	and return their values on the value stack.
*/

#include "machine.h"

#include STRING_H

#include "glo.h"
#include "micro.h"
#include "memory.h"
#include "error.h"
#include "ops.h"
#include "predefs.h"
#include "eval.h"
#include "compile.h"

/* ----------------------------------------------------------------------- */
/*                      Special Compiler Operations			   */
/* ----------------------------------------------------------------------- */

/* opNoOp() - Do nothing. */
void opNoOp()
{
   assert(0);
}

/* opMakeClosure() - Create a closure. */
void opMakeClosure()
{
   CONS close;

   close = NewCons( CLOSURE, 0, 0 );
   mcCl_Env(close) = mcGet_Nested(glo_env);
   mcCl_Body(close) = mcPopVal();
   mcCl_Parms(close) = mcPopVal();
   mcPushVal(close);
}

/* ----------------------------------------------------------------------- */
/*                       Interpreter Directives				   */
/* ----------------------------------------------------------------------- */

/* opEnv() - Returns the current environment. */
void opEnv()
{
   mcPushVal( glo_env );
}

/* opExit() - Quit scheme. */
void opExit()
{
   exit(0);
}

/* (TORTURE) - Turn torture test on/off.  Returns the new state of torture
	test.
*/
void opTorture()
{
   /* if torture is on, turn it off */
   if ( torture == TRUE ) {
	torture = FALSE;
	mcPushVal( NIL );
	return;
   }

   /* torture is off, turn it on */
   torture = TRUE;
   mcPushVal( T );
}

/* (GCDEBUG) - Turns GC debugging on/off.  Returns the new state of
	debugging.
*/
void opGcDebug()
{
   /* gc_debugging is on, turn it off */
   if ( gc_debug == TRUE ) {
	gc_debug = FALSE;
	mcPushVal( NIL );
	return;
   }

   /* gc_debugging is off, turn it on */
   gc_debug = TRUE;
   mcPushVal( T );
}

/* (EVDEBUG) - Turns evaluation debugging on/off.  Returns the new state. */
void opEvDebug()
{
   /* eval debugging is on, turn it off */
   if ( eval_debug == TRUE ) {
	eval_debug = FALSE;
	mcPushVal( NIL );
	return;
   }

   /* eval debugging is off, turn it on */
   eval_debug = TRUE;
   mcPushVal( T );
}

/* ----------------------------------------------------------------------- */
/*                            Evaluation Ops				   */
/* ----------------------------------------------------------------------- */

/* (EVAL expr env) */
void opEval()
{
   CONS exp, env;

   exp = mcPopVal();
   env = mcPopVal();

   /* env is environment to evaluate 'exp' in */
   if ( env != MARK ) {

	if ( !mcEnvironment( env ) ) {
		RT_LERROR("EVAL: Not an environment: ", env);
	}

	/* pop MARK */
	(void)mcPopVal();

	/* save the current environment */
	evSaveEnv();
	glo_env = env;
   }

   mcPushExpr( exp );
   return;
}

/* (APPLY func arg-list) */
void opApply()
{
   CONS func, args;

   func = mcPopVal();
   args = mcPopVal();

   /* setup to invoke the function */
   evCallFunc( func, args );
}

/* (CALL/CC func) */
void opCallCC()
{
   ENTER;
   REG(cont);
   REG(func);

   R(func) = mcPopVal();

   /* build the continuation */
   R(cont) = NewCons( CONT, 0, 0 );
   mcCont_Env( R(cont) ) = mcGet_Nested(glo_env);
   mcCont_Val( R(cont) ) = mcGetValS();
   mcCont_Exp( R(cont) ) = mcGetExprS();
   mcCont_Fnc( R(cont) ) = mcGetFuncS();

   /* evCallFunc takes the list of arguments so put the continuation
    * into a list with only 1 element.
    */
   R(cont) = mcCons( R(cont), NIL );

   /* setup to invoke the function */
   evCallFunc( R(func), R(cont) );

   OPVOIDLEAVE;
}

/* (COMPILE exp) */
void opCompile()
{
   ENTER;
   REG(exp);
   REG(code);

   R(exp) = mcPopVal();
   R(code) = mcCompile( R(exp) );

   OPLEAVE( R(code) );
}

#ifdef OLD_CODE
/* opMap(args) - (MAP PROC LIST1 ...)
	Builds resume: (RESUME prMap PROC RESULT LIST1 ...)
*/
void opMap(args)
CONS args;
{
   ENTER;
   REG(parms);

   R(parms) = mcCdr(args);
   R(parms) = mcRev( R(parms) );

   opDoMap( mcCar(args), R(parms), NIL );

   OPVOIDLEAVE;
}

void opResMap(args)
CONS args;
{
   ENTER;
   REG(proc);
   REG(parms);
   REG(result);

   R(proc) = mcCaddr(args);
   R(result) = mcCadddr(args);
   R(parms) = mcCddddr(args);

   /* add result of last evaluation */
   R(result) = mcCons( mcPopVal(), R(result) );

   /* done evaluating? */
   if ( mcNull( mcCar(R(parms)) ) ) {
	R(result) = mcRev( R(result) );
	OPLEAVE( R(result) );
   }

   opDoMap( R(proc), R(parms), R(result) );
   OPVOIDLEAVE;
}
#endif

/* ----------------------------------------------------------------------- */
/*                       Primitive List Operations			   */
/* ----------------------------------------------------------------------- */

/* (CAR obj) */
void opCar()
{
   CONS lyst;

   lyst = mcPopVal();

#ifdef PURE_CAR_CDR
   if ( mcNull( lyst ) )
	RT_ERROR("CAR: Can't take car of NULL.");
#endif
   mcPushVal( mcCar(lyst) );
}

/* (CDR obj) */
void opCdr()
{
   CONS lyst;

   lyst = mcPopVal();

#ifdef PURE_CAR_CDR
   if ( mcNull(lyst) )
	RT_ERROR("CDR: Can't take CDR of NULL.");
#endif
   mcPushVal( mcCdr(lyst) );
}

/* (CONS obj1 obj2) */
void opCons()
{
   ENTER;
   REG(head);
   REG(tail);
   REG(lyst);

   R(head) = mcPopVal();
   R(tail) = mcPopVal();

   R(lyst) = mcCons( R(head), R(tail) );

   OPLEAVE( R(lyst) );
}

/* (SET-CAR! obj1 obj2) */
void opSetCar()
{
   CONS c, h;

   c = mcPopVal();
   h = mcPopVal();

   if ( !mcPair(c) )
	RT_ERROR("SET-CAR!: First arg must be a pair.");

   mcPushVal( mcSetCar(c, h) );
}

/* (SET-CDR! obj1 obj2) */
void opSetCdr()
{
   CONS c, t;

   c = mcPopVal();
   t = mcPopVal();

   if ( !mcPair(c) )
	RT_ERROR("SET-CDR!: First arg must be a pair.");

   mcPushVal( mcSetCdr(c, t) );
}

/* ----------------------------------------------------------------------- */
/*                      Miscellaneous Functions				   */
/* ----------------------------------------------------------------------- */

/* (NOT boolean) */
void opNot()
{
   CONS l;

   l = mcPopVal();
   if ( mcNull(l) || l == F ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* ----------------------------------------------------------------------- */
/*                      Higher Level List Operations			   */
/* ----------------------------------------------------------------------- */

/* (ASSOC key a-list) */
void opAssoc()
{
   CONS symbol, alist;

   symbol = mcPopVal();
   alist = mcPopVal();

   mcPushVal( mcAssoc(symbol, alist) );
}

/* (ASSQ key a-list) */
void opAssq()
{
   CONS symbol, alist;

   symbol = mcPopVal();
   alist = mcPopVal();

   mcPushVal( mcAssq(symbol, alist) );
}

/* (ASSV key a-list) */
void opAssv()
{
   CONS symbol, alist;

   symbol = mcPopVal();
   alist = mcPopVal();

   mcPushVal( mcAssv(symbol, alist) );
}

/* (MEMBER key list) */
void opMember()
{
   CONS obj, lyst;

   obj = mcPopVal();
   lyst = mcPopVal();

   mcPushVal( mcMember(obj, lyst) );
}

/* (MEMQ key list) */
void opMemq()
{
   CONS obj, lyst;

   obj = mcPopVal();
   lyst = mcPopVal();

   mcPushVal( mcMemq(obj, lyst) );
}

/* (MEMV key list) */
void opMemv()
{
   CONS obj, lyst;

   obj = mcPopVal();
   lyst= mcPopVal();

   mcPushVal( mcMemv(obj, lyst) );
}

/* (LIST obj1 obj2 ...) */
void opList()
{
   CONS l;

   l = evGatherVal();
   mcPushVal( l );
}

/* (REVERSE list) */
void opRev()
{
   ENTER;
   REG(arg);

   R(arg) = mcPopVal();
   R(arg) = mcTree_Copy( R(arg) );
   R(arg) = mcRev( R(arg) );

   OPLEAVE( R(arg) );
}

/* (APPEND list1 list2 list3 ...) */
void opAppend()
{
   ENTER;
   REG(res);
   REG(sarg);
   REG(arg);
   REG(rptr);

   /* this is kind of ugly, but given list1, list2, ...,listn, i want
    * append to be O( |list1|+|list2|+|...| ).  the fact that '() acts
    * as a list here and not as an atom complicates it a little.  also,
    * getting the lists in order makes it a little more tricky.  it's
    * easier when you get them in reverse order (like the Scheme code).
    */

   /* get first arg, skip leading '()s */
   R(sarg) = R(arg) = mcPopVal();
   while ( mcNull( R(arg) ) )
	R(sarg) = R(arg) = mcPopVal();

   /* (append '() '()) => () */
   if ( R(arg) == MARK ) {
	OPLEAVE( NIL );
   }

   /* initialize result to be (cons (car arg) '()) */
   if ( !mcNull(R(arg)) && !mcPair( R(arg) ) ) {
	RT_LERROR("APPEND: Requires lists: ", R(arg) );
   }
   R(res) = mcCons( mcCar(R(arg)), NIL );
   R(rptr) = R(res);

   /* we've copied the car of arg, copy the rest of it */
   R(arg) = mcCdr( R(arg) );
   goto complete;

   while ( TRUE ) {
	/* pop first arg */
	R(sarg) = R(arg) = mcPopVal();

	/* that good old '() throws things off a little */
	if ( R(arg) == MARK ) {
		OPLEAVE( R(res) );
	}

	if ( !mcNull(R(arg)) && !mcPair(R(arg)) ) {
		RT_LERROR("APPEND: Requires lists: ", R(arg));
	}

	/* last arg just gets SET-CDR! */
	if ( mcValStackTop() == MARK ) {
		mcSetCdr( R(rptr), R(arg) );
		mcPopVal();
		OPLEAVE( R(res) );
	}

complete:
	/* append it */
     {
	CONS curr;

	for ( curr = R(arg); !mcNull(curr); curr = mcCdr(curr) ) {
		mcSetCdr( R(rptr), mcCons( mcCar(curr), NIL ) );
		R(rptr) = mcCdr( R(rptr) );
	}
     }
   }
}

/* (TREE-COPY list) */
void opTree_Copy()
{
   ENTER;
   REG(result);

   R(result) = mcPopVal();
   R(result) = mcTree_Copy( R(result) );
   OPLEAVE( R(result) );
}

/* (LENGTH list) */
void opLength()
{
   CONS l;

   l = mcPopVal();
   l = mcIntToCons( mcLength(l) );
   mcPushVal(l);
}

/* ----------------------------------------------------------------------- */
/*                        Character Operations				   */
/* ----------------------------------------------------------------------- */

/* (CHAR->INTEGER char) */
void opCharInt()
{
   CONS ch;

   ch = mcPopVal();
   if ( !mcChar(ch) )
	RT_ERROR("CHAR->INTEGER: Arg must be a character.");

   mcPushVal( mcIntToCons( (int) mcGet_Char(ch) ) );
}

/* (INTEGER->CHAR int) */
void opIntChar()
{
   CONS num;

   num = mcPopVal();
   if ( !mcInteger(num) )
	RT_ERROR("INTEGER->CHAR: Arg must be an integer.");

   if ( mcGet_Int(num) < 0 || mcGet_Int(num) > 255 ) {
	mcPushVal( mcCharToCons( (int)'\000' ) );
	return;
   }

   mcPushVal( mcCharToCons( mcGet_Int(num) ) );
}

/* ----------------------------------------------------------------------- */
/*                         String Operations				   */
/* ----------------------------------------------------------------------- */

/* (STRING-LENGTH string) */
void opStrLen()
{
   int len;
   CONS str;

   str = mcPopVal();
   if ( !mcString(str) )
	RT_ERROR("STRING-LENGTH: Arg must be a string.");

   len = strlen( mcGet_Str(str) );
   mcPushVal( mcIntToCons(len) );
}

/* (STRING-REF string k) */
void opStrRef()
{
   int index;
   char *s;
   CONS ref, str;

   str = mcPopVal();
   if ( !mcString(str) )
	RT_LERROR("STRING-REF: First arg must be a string: ", str);

   ref = mcPopVal();
   if ( !mcInteger(ref) )
	RT_LERROR("STRING-REF: Second arg must be an integer: ", ref);

   s = mcGet_Str(str);
   if ( (index = mcGet_Int(ref)) > strlen(s)-1 )
	RT_LERROR("STRING-REF: REF is greater than string length: ", ref);

   mcPushVal( mcCharToCons( (int)*(s+index) ) );
}

/* (SUBSTRING string start end) */
void opSubStr()
{
   int len, beg, end;
   CONS str, start, stop;

   str = mcPopVal();
   if ( !mcString(str) )
	RT_LERROR("SUBSTRING: First arg must be a string: ", str);

   start = mcPopVal();
   if ( !mcInteger(start) )
	RT_LERROR("SUBSTRING: Second arg must be an integer: ", start);

   stop = mcPopVal();
   if ( !mcInteger(stop) )
	RT_LERROR("SUBSTRING: Third arg must be an integer: ", stop);

   len = strlen( mcGet_Str(str) ) - 1;
   if ( (beg = mcGet_Int(start)) > len )
	RT_ERROR("SUBSTRING: START > string length.");

   if ( (end = mcGet_Int(stop)) > len )
	RT_ERROR("SUBSTRING: STOP > string length.");

   mcPushVal( mcSubStr( mcGet_Str(str), beg, end ) );
}

/* (STRING-APPEND str1 str2) */
void opStrApp()
{
   CONS str1, str2;

   str1 = mcPopVal();
   if ( !mcString(str1) )
	RT_LERROR("STRING-APPEND: Args must be strings: ", str1);

   str2 = mcPopVal();
   if ( !mcString(str2) )
	RT_LERROR("STRING-APPEND: Args must be strings: ", str2);

   mcPushVal( mcStrApp( mcGet_Str(str1), mcGet_Str(str2) ) );
}

/* (STRING->LIST string) */
void opStrLst()
{
   CONS str;

   str = mcPopVal();
   if ( !mcString(str) )
	RT_LERROR("STRING->LIST: Arg must be a string: ", str);

   mcPushVal( mcStrLst( mcGet_Str(str) ) );
}

/* (LIST->STRING chars) */
void opLstStr()
{
   ENTER;
   REG(lst);

   R(lst) = mcPopVal();
   if ( !mcPair( R(lst) ) )
	RT_LERROR("LIST->STRING: Arg must be a list: ", R(lst) );

   R(lst) = mcLstStr( R(lst) );
   OPLEAVE( R(lst) );
}

/* (SYMBOL->STRING symbol) */
void opSymStr()
{
   CONS sym;

   sym = mcPopVal();
   if ( !mcSymbol(sym) )
	RT_LERROR("SYMBOL->STRING: Arg must be a symbol: ", sym);

   mcPushVal( mcSymStr(sym) );
}

/* (STRING->SYMBOL string) */
void opStrSym()
{
   CONS str;

   str = mcPopVal();
   if ( !mcString(str) )
	RT_LERROR("STRING->SYMBOL: Arg must be a string: ", str);

   mcPushVal( mcStrSym(str) );
}

/* (GENSYM) */
void opGenSym()
{
   CONS s;

   s = mcGenSym();
   mcPushVal( s );
}

/* ----------------------------------------------------------------------- */
/*                           Vector Operations				   */
/* ----------------------------------------------------------------------- */

/* (VECTOR obj ...) */
void opArgVector()
{
   CONS v;

   v = mcArgVector();
   mcPushVal( v );
}

/* (MAKE-VECTOR n obj) */
void opMakeVector()
{
   ENTER;
   REG(n);
   REG(obj);
   REG(v);

   R(n) = mcPopVal();
   R(obj) = mcPopVal();

   if ( !mcNumber(R(n)) || mcGet_Int(R(n)) < 0 )
	RT_LERROR("MAKE-VECTOR: Requires a non-negative number:", R(n));

   R(v) = mcMakeVector( mcGet_Int(R(n)), R(obj) );

   OPLEAVE( R(v) );
}

/* (VECTOR-LENGTH v) */
void opVectLength()
{
   CONS s, v;

   v = mcPopVal();
   if ( !mcVector(v) )
	RT_LERROR("VECTOR-LENGTH: Requires a vector: ", v);

   s = mcIntToCons( mcVect_Size(v) );
   mcPushVal( s );
}

/* (VECTOR-REF v n) */
void opVectRef()
{
   CONS v, n;
   int i;

   v = mcPopVal();
   n = mcPopVal();

   if ( !mcVector(v) )
	RT_LERROR("VECTOR-REF: Requires a vector: ", v);

   if ( !mcNumber(n) || mcGet_Int(n) < 0 || mcGet_Int(n) >= mcVect_Size(v) )
	RT_LERROR("VECTOR-REF: Illegal reference: ", n);

   i = mcGet_Int(n);
   mcPushVal( *mcVect_Ref(v, i) );
}

/* (VECTOR-SET! v n obj) */
void opVectSet()
{
   CONS v, n, obj;
   int i;

   v = mcPopVal();
   n = mcPopVal();
   obj = mcPopVal();

   if ( !mcVector(v) )
	RT_LERROR("VECTOR-SET!: Requires a vector: ", v);

   if ( !mcNumber(n) || mcGet_Int(n) < 0 || mcGet_Int(n) >= mcVect_Size(v) )
	RT_LERROR("VECTOR-SET!: Illegal reference: ", n);

   i = mcGet_Int(n);
   *mcVect_Ref(v, i) = obj;

   mcPushVal( v );
}

/* (VECTOR-COPY v) */
void opVectCopy()
{
   ENTER;
   REG(old);
   REG(new);

   R(old) = mcPopVal();

   if ( !mcVector( R(old) ) )
	RT_LERROR("VECTOR-COPY: Arg must be a vector: ", R(old) );

   R(new) = mcVectorCopy( R(old) );

   OPLEAVE( R(new) );
}

/* (VECTOR-FILL! v obj) */
void opVectFill()
{
   CONS v, obj;

   v = mcPopVal();
   obj = mcPopVal();

   if ( !mcVector(v) )
	RT_LERROR("VECTOR-FILL!: Requires a vector: ", v);

   mcVectorFill(v, obj);
   mcPushVal( v );
}

/* (VECTOR->LIST v) */
void opVectLst()
{
   ENTER;
   REG(v);

   R(v) = mcPopVal();
   R(v) = mcVectorLst( R(v) );

   OPLEAVE( R(v) );
}

/* (LIST->VECTOR l) */
void opLstVect()
{
   ENTER;
   REG(l);

   R(l) = mcPopVal();
   R(l) = mcLstVector( R(l) );

   OPLEAVE( R(l) );
}

/* ----------------------------------------------------------------------- */
/*                            Port Operations				   */
/* ----------------------------------------------------------------------- */

/* (CURRENT-INPUT-PORT) */
void opCurrIn()
{
   mcPushVal( STDIN );
}

/* (CURRENT-OUTPUT-PORT) */
void opCurrOut()
{
   mcPushVal( STDOUT );
}

/* (READ port) */
void opRead()
{
   ENTER;
   REG(obj);
   REG(port);

   if ( (R(port) = mcPopVal()) == MARK ) {
	R(obj) = mcRead(currin);
	OPLEAVE( R(obj) );
   }

   /* pop MARK */
   mcPopVal();

   if ( !mcPort(R(port)) )
	RT_LERROR("READ: Arg must be a port.", R(port));

   if ( mcGet_PortType( R(port) ) != INPUT )
	RT_ERROR("READ: Port must be an input port.");

   R(obj) = mcRead( mcGet_Port(R(port)) );
   OPLEAVE( R(obj) );
}

/* (WRITE obj port) */
void opWrite()
{
   CONS obj, port;

   /* pop object to write */
   obj = mcPopVal();

   /* if there is a second argument, direct output to that port */
   if ( (port = mcPopVal()) != MARK ) {
	/* pop MARK */
	(void)mcPopVal();

	/* second arg better be a port */
	if ( !mcPort(port) )
		RT_LERROR("WRITE: Second arg must be a port.", port);

	if ( mcGet_PortType(port) != OUTPUT )
		RT_ERROR("WRITE: Port must be an output port.");

	mcWrite( obj, mcGet_Port(port) );
   } else mcWrite( obj, currout );

   mcPushVal( obj );
   return;
}

/* (NEWLINE port) */
void opNewLine()
{
   CONS port;

   if ( (port = mcPopVal()) == MARK )
	fprintf(currout, "\n");
   else {
	/* pop MARK */
	mcPopVal();

	if ( !mcPort(port) )
		RT_LERROR("NEWLINE: Arg must be a port: ", port);

	fprintf( mcGet_Port(port), "\n" );
   }

   mcPushVal( NIL );
}

/* (READ-CHAR port) */
void opReadChar()
{
   int ch;
   CONS port;

   if ( (port = mcPopVal()) == MARK )
	ch = fgetc(currin);
   else {
	/* pop MARK */
	mcPopVal();

	if ( !mcPort(port) )
		RT_LERROR("READ-CHAR: Arg must be a port: ", port);

	if ( mcGet_PortType(port) != INPUT )
		RT_ERROR("READ-CHAR: Port must be an input port.");

	ch = getc( mcGet_Port(port) );
   }

   mcPushVal( mcCharToCons(ch) );
}

/* (WRITE-CHAR char port) */
void opWriteChar()
{
   CONS ch, port;

   /* pop character */
   ch = mcPopVal();

   if ( !mcChar(ch) )
	RT_LERROR("WRITE-CHAR: First arg must be a character: ", ch );

   if ( (port = mcPopVal()) != MARK ) {
	/* pop MARK */
	(void)mcPopVal();

	if ( !mcPort(port) )
		RT_LERROR("WRITE-CHAR: Second arg must be a port: ", port);

	if ( mcGet_PortType( port ) != OUTPUT )
		RT_ERROR("WRITE-CHAR: Port must be an output port.");

	fprintf( mcGet_Port(port), "%c", mcGet_Char(ch) );
   } else {
	fprintf( currout, "%c", mcGet_Char(ch) );
   }

   mcPushVal( ch );
}

/* (DISPLAY obj port) */
void opDisplay()
{
   CONS port, obj;

   obj = mcPopVal();

   if ( (port = mcPopVal()) != MARK ) {
	/* pop MARK */
	(void)mcPopVal();

	if ( !mcPort(port) )
		RT_LERROR("DISPLAY: Second arg must be a port.\n", port);

	if ( mcGet_PortType(port) != OUTPUT )
		RT_ERROR("DISPLAY: Port must be an ouput port.");

	mcDisplay(obj, mcGet_Port(port));
   } else mcDisplay(obj, currout);

   mcPushVal( obj );
}

/* (OPEN-INPUT-FILE name) */
void opOpenInFile()
{
   FILE *ifp;
   {
   ENTER;
   REG(name);
   REG(port);

   R(name) = mcPopVal();

   if ( !mcString( R(name) ) )
	RT_LERROR("OPEN-INPUT-FILE: First arg must be a string: ", R(name));

   /* open the file */
   if ( (ifp = fopen( mcGet_Str(R(name)), "r")) == NULL )
	RT_LERROR("OPEN-INPUT-FILE: Can't open: ", R(name) );

   /* create a port */
   R(port) = NewCons( PORT, 0, 0 );
   mcCpy_Port( R(port), ifp );
   mcCpy_PortType( R(port), INPUT );

   OPLEAVE( R(port) );
   }
}

/* (OPEN-OUTPUT-FILE name) */
void opOpenOutFile()
{
   FILE *ifp;
   {
   ENTER;
   REG(name);
   REG(port);

   R(name) = mcPopVal();

   if ( !mcString( R(name) ) )
	RT_LERROR("OPEN-OUTPUT-FILE: First arg must be a string:", R(name));

   /* open the file */
   if ( (ifp = fopen( mcGet_Str(R(name)), "w")) == NULL )
	RT_LERROR("OPEN-OUTPUT-FILE: Can't open: ", R(name) );

   /* create a port */
   R(port) = NewCons( PORT, 0, 0 );
   mcCpy_Port( R(port), ifp );
   mcCpy_PortType( R(port), OUTPUT );

   OPLEAVE( R(port) );
   }
}

/* (CLOSE port) */
void opClose()
{
   CONS port;

   port = mcPopVal();

   if ( !mcPort(port) )
	RT_LERROR("CLOSE: Arg must be a port: ", port);

   fclose( mcGet_Port(port) );
   mcCpy_PortType( port, CLOSED );

   /* gee, what should be returned? */
   mcPushVal( NIL );
}

/* (LOAD string) */
void opLoad()
{
   ENTER;
   REG(name);

   /* this operation is a little tricky since it's got to recursively
    * invoke eval on each expression in the file, but we can't invoke
    * eval until we return but we can't return until we've loaded the
    * file.  the trick is to push the return value since we'll never
    * return here, THEN have the microcode recur using RESUMEs.  ``it's
    * all in the timing.''
    */
   R(name) = mcPopVal();
   mcPushVal( R(name) );

   if ( !mcString(R(name)) )
	RT_LERROR("LOAD: Arg must be a string: ", R(name));

   if ( !mcLoad( mcGet_Str( R(name)) ) )
	RT_LERROR("LOAD: File not found: ", R(name));

   OPVOIDLEAVE;
}

/* ----------------------------------------------------------------------- */
/*                              Math Operations				   */
/* ----------------------------------------------------------------------- */

/* (+ n1 ...) */
void opPlus()
{
   CONS r;

   r = mcPlus();
   mcPushVal(r);
}

/* (* n1 ...) */
void opMult()
{
   CONS r;

   r = mcMult();
   mcPushVal(r);
}

/* (- n1 ...) */
void opMinus()
{
   CONS r;

   r = mcMinus();
   mcPushVal(r);
}

/* (/ num1 num2 ...) */
void opDiv()
{
   CONS r;

   r = mcDiv();
   mcPushVal(r);
}

/* (ABS num) */
void opAbs()
{
   CONS r;

   r = mcAbs();
   mcPushVal(r);
}

/* ----------------------------------------------------------------------- */
/*                         Evaluation Operations			   */
/* ----------------------------------------------------------------------- */

/* (ERROR obj1 ...) */
void opError()
{
   CONS curr;

   /* display args */
   while ( (curr = mcPopVal()) != MARK ) {
	mcDisplay(curr, currout);
	fprintf(currout, " ");
   }

   ERROR;
}

/* ----------------------------------------------------------------------- */
/*                           Environment Funcs				   */
/* ----------------------------------------------------------------------- */

/* (DUMP-ENVIRONMENT filename) */
void opDumpEnv()
{
   ENTER;
   REG(name);
   FILE *fp;

   R(name) = mcPopVal();
   if ( !mcString(R(name)) ) {
	RT_LERROR("DUMP-ENVIRONMENT: Arg must be a string: ", R(name));
   }

   if ( (fp = fopen( mcGet_Str(R(name)), FILE_WRITE_BIN )) == NULL ) {
	RT_LERROR("DUMP-ENVIRONMENT: Filename not found: ", R(name) );
   }

   if ( mcDumpEnv(fp) == FALSE ) {
	OPLEAVE( F );
   }

   OPLEAVE( R(name) );
}

/* (RESTORE-ENVIRONMENT filename) */
void opRestEnv()
{
   ENTER;
   REG(name);
   FILE *fp;

   R(name) = mcPopVal();
   if ( !mcString(R(name)) ) {
	RT_LERROR("RESTORE-ENVIRONMENT: Arg must be a string: ", R(name));
   }

   if ( (fp = fopen( mcGet_Str(R(name)), FILE_READ_BIN )) == NULL ) {
	RT_LERROR("RESTORE-ENVIRONMENT: Filename not found: ", R(name) );
   }

   if ( mcRestEnv(fp) == FALSE ) {
	OPLEAVE( F );
   }

   OPLEAVE( R(name) );
}

/* ----------------------------------------------------------------------- */
/*                        OS Specific Operations			   */
/* ----------------------------------------------------------------------- */

/* (CHDIR string) */
void opChdir()
{
   CONS l;

   l = mcPopVal();
   if ( !mcString(l) )
	RT_LERROR("CHDIR: Requires a string: ", l);

   if ( chdir( mcGet_Str(l) ) )
	mcPushVal( F );
   else mcPushVal( l );
}
