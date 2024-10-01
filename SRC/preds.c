/* preds.c - Scheme predicates.
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
#include "preds.h"

/* (BOOLEAN? obj) */
void opBoolean()
{
   CONS l;

   l = mcPopVal();
   if ( l == T || l == F ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (NULL? obj) */
void opNull()
{
   CONS l;

   l = mcPopVal();
   if ( mcNull(l) || l == F ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (ATOM? obj) */
void opAtom()
{
   CONS l;

   l = mcPopVal();
   if ( mcAtom(l) ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (PAIR? obj) */
void opPair()
{
   CONS l;

   l = mcPopVal();
   if ( mcPair(l) ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (SYMBOL? obj) */
void opSymbol()
{
   CONS l;

   l = mcPopVal();
   if ( mcSymbol(l) ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (PROCEDURE? obj) */
void opProcedure()
{
   CONS p;

   p = mcPopVal();
   mcPushVal( mcProcedure(p) );
}

/* (VECTOR? obj) */
void opVector()
{
   CONS p;

   p = mcPopVal();
   if ( mcVector(p) )
	mcPushVal( T );
   else
	mcPushVal( F );

   return;
}

/* ----------------------------------------------------------------------- */
/*                           Number Predicates				   */
/* ----------------------------------------------------------------------- */

/* (NUMBER? obj) */
void opNumber()
{
   CONS l;

   l = mcPopVal();
   if ( mcNumber(l) ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (INTEGER? obj) */
void opInteger()
{
   CONS l;

   l = mcPopVal();
   if ( mcInteger(l) ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (FLOAT? obj) */
void opFloat()
{
   CONS l;

   l = mcPopVal();
   if ( mcFloat(l) ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (ZERO? obj) */
void opZero()
{
   CONS l;

   l = mcPopVal();
   if ( mcZero(l) ) {
	mcPushVal( T );
	return;
   }

   mcPushVal( F );
}

/* (= n1 n2 ...) */
void opE()
{
   CONS l;

   l = mcE();
   mcPushVal( l );
}

/* (< n1 n2 ...) */
void opLT()
{
   CONS l;

   l = mcLT();
   mcPushVal( l );
}

/* (> n1 n2 ...) */
void opGT()
{
   CONS l;

   l = mcGT();
   mcPushVal( l );
}

/* (<= n1 n2 ...) */
void opLTE()
{
   CONS l;

   l = mcLTE();
   mcPushVal( l );
}

/* (>= n1 n2 ...) */
void opGTE()
{
   CONS l;

   l = mcGTE();
   mcPushVal( l );
}

/* (<> n1 n2 ...) */
void opNE()
{
   CONS l;

   l = mcNE();
   mcPushVal( l );
}

/* ----------------------------------------------------------------------- */
/*                            List Predicates				   */
/* ----------------------------------------------------------------------- */

/* (EQUAL? obj1 obj2) */
void opEqual()
{
   CONS l1, l2;

   l1 = mcPopVal();
   l2 = mcPopVal();

   mcPushVal( mcEqual(l1, l2) );
}

/* (EQ? obj1 obj2) */
void opEq()
{
   CONS l1, l2;

   l1 = mcPopVal();
   l2 = mcPopVal();

   mcPushVal( mcEq(l1, l2) );
}

/* (EQV? obj1 obj2) */
void opEqv()
{
   CONS l1, l2;

   l1 = mcPopVal();
   l2 = mcPopVal();

   mcPushVal( mcEqv(l1, l2) );
}

/* ----------------------------------------------------------------------- */
/*                          Character Predicates			   */
/* ----------------------------------------------------------------------- */

/* (CHAR? obj) */
void opChar()
{
   CONS ch;

   ch = mcPopVal();
   mcPushVal( ( mcChar(ch) ? T : F ) );
}

/* (CHAR=? char1 char2) */
void opCharE()
{
   CONS ch1, ch2;

   ch1 = mcPopVal();
   ch2 = mcPopVal();
   if ( !mcChar(ch1) )
	RT_ERROR("CHAR=?: Args must be characters.");

   if ( !mcChar(ch2) )
	RT_ERROR("CHAR=?: Args must be characters.");

   mcPushVal( ( mcCharE(ch1, ch2) ? T : F ) );
}

/* (CHAR<? char1 char2) */
void opCharL()
{
   CONS ch1, ch2;

   ch1 = mcPopVal();
   ch2 = mcPopVal();
   if ( !mcChar(ch1) )
	RT_ERROR("CHAR<?: Args must be characters.");

   if ( !mcChar(ch2) )
	RT_ERROR("CHAR<?: Args must be characters.");

   mcPushVal( ( mcCharL(ch1, ch2) ? T : F ) );
}

/* (CHAR>? char1 char2) */
void opCharG()
{
   CONS ch1, ch2;

   ch1 = mcPopVal();
   ch2 = mcPopVal();
   if ( !mcChar(ch1) )
	RT_ERROR("CHAR>?: Args must be characters.");

   if ( !mcChar(ch2) )
	RT_ERROR("CHAR>?: Args must be characters.");

   mcPushVal( ( mcCharG(ch1, ch2) ? T : F ) );
}

/* (CHAR<=? char1 char2) */
void opCharLE()
{
   CONS ch1, ch2;

   ch1 = mcPopVal();
   ch2 = mcPopVal();
   if ( !mcChar(ch1) )
	RT_ERROR("CHAR<=?: Args must be characters.");

   if ( !mcChar(ch2) )
	RT_ERROR("CHAR<=?: Args must be characters.");

   mcPushVal( ( mcCharLE(ch1, ch2) ? T : F ) );
}

/* (CHAR>=? char1 char2) */
void opCharGE()
{
   CONS ch1, ch2;

   ch1 = mcPopVal();
   ch2 = mcPopVal();
   if ( !mcChar(ch1) )
	RT_ERROR("CHAR>=?: Args must be characters.");

   if ( !mcChar(ch2) )
	RT_ERROR("CHAR>=?: Args must be characters.");

  mcPushVal( ( mcCharGE(ch1, ch2) ? T : F ) );
}

/* ----------------------------------------------------------------------- */
/*                           String Predicates				   */
/* ----------------------------------------------------------------------- */

/* (STRING? obj) */
void opString()
{
   CONS ch;

   ch = mcPopVal();
   mcPushVal( ( mcString(ch) ? T : F ) );
}

/* (STRING=? str1 str2) */
void opStrE()
{
   CONS str1, str2;

   str1 = mcPopVal();
   str2 = mcPopVal();
   if ( !mcString(str1) )
	RT_LERROR("STRING=?: Args must be strings: ", str1);

   if ( !mcString(str2) )
	RT_LERROR("STRING=?: Args must be strings: ", str2);

   mcPushVal( ( mcStrE(str1, str2) ? T : F ) );
}

/* (STRING<? str1 str2) */
void opStrL()
{
   CONS str1, str2;

   str1 = mcPopVal();
   str2 = mcPopVal();
   if ( !mcString(str1) )
	RT_LERROR("STRING<?: Args must be strings: ", str1);

   if ( !mcString(str2) )
	RT_LERROR("STRING<?: Args must be strings: ", str2);

   mcPushVal( ( mcStrL(str1, str2) ? T : F ) );
}

/* (STRING>? str1 str2) */
void opStrG()
{
   CONS str1, str2;

   str1 = mcPopVal();
   str2 = mcPopVal();
   if ( !mcString(str1) )
	RT_LERROR("STRING>?: Args must be strings: ", str1);

   if ( !mcString(str2) )
	RT_LERROR("STRING>?: Args must be strings: ", str2);

   mcPushVal( ( mcStrG(str1, str2) ? T : F ) );
}

/* (STRING<=? str1 str2) */
void opStrLE()
{
   CONS str1, str2;

   str1 = mcPopVal();
   str2 = mcPopVal();
   if ( !mcString(str1) )
	RT_LERROR("STRING<=?: Args must be strings: ", str1);

   if ( !mcString(str2) )
	RT_LERROR("STRING<=?: Args must be strings: ", str2);

   mcPushVal( ( mcStrLE(str1, str2) ? T : F ) );
}

/* (STRING>=? str1 str2) */
void opStrGE()
{
   CONS str1, str2;

   str1 = mcPopVal();
   str2 = mcPopVal();

   if ( !mcString(str1) )
	RT_LERROR("STRING>=?: Args must be strings: ", str1);

   if ( !mcString(str2) )
	RT_LERROR("STRING>=?: Args must be strings: ", str2);

   mcPushVal( ( mcStrGE(str1, str2) ? T : F ) );
}

/* ----------------------------------------------------------------------- */
/*                            Port Predicates				   */
/* ----------------------------------------------------------------------- */

/* (EOF-OBJECT? obj) */
void opEofObj()
{
   CONS a;

   a = mcPopVal();
   mcPushVal( ( a == EOF_OBJ ? T : F ) );
}

/* (INPUT-PORT? obj) */
void opInPort()
{
   CONS p;

   p = mcPopVal();
   if ( !mcPort(p) )
	RT_LERROR("INPUT-PORT?: Requires a port: ", p);

   mcPushVal( (mcGet_PortType(p) == INPUT ? T : F) );
}

/* (OUTPUT-PORT? obj) */
void opOutPort()
{
   CONS p;

   p = mcPopVal();
   if ( !mcPort(p) )
	RT_LERROR("INPUT-PORT?: Requires a port: ", p);

   mcPushVal( (mcGet_PortType(p) == OUTPUT ? T : F) );
}
