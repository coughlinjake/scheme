/* Symstr.c - The Symbol Table & String Routines

   Version 1

	- The symbol table is a closed hash table.
	   - All symbols are stored only ONCE.
	   - Neither symbols or strings are removed from the symbol table
	     during GC.  Therefore, it is possible to fill the table with
	     garbage.
	   - Symbols are actually stored in the string space.

	- Distinction between "symbol" and "string".  "symbol" is a Scheme
	identifier; you bind values to symbols.  "string" is a Scheme string;
	it is an array of characters.

   BUGS:

	- String space is allocated and never deallocated.  Strings which are
	GCed are lost (since strings aren't hashed into a table of pointers)
	and their memory is not reclaimed.  Probably should be fixed, but
	requires more sophisticated string management (one with	reference
	counts).
*/

#include "machine.h"

#include STDLIB_H
#include STRING_H

#include "glo.h"
#include "error.h"
#include "micro.h"
#include "symstr.h"

#define STRING_SIZE	5000	/* bytes */

/* globals */
#ifdef DEBUG_SYM
	int sym_debug = TRUE;
#else
	int sym_debug = FALSE;
#endif

char *SymTable[MAX_SYMBOLS];	/* symbol table */
char *STRING_SPACE;		/* string storage space */
int Sleft;			/* # bytes left in string storage space */

/* InitSymstr() - Initialize the symbol table and string routines. */
void InitSymstr()
{
   int i;

   /* allocate string space */
   STRING_SPACE = (char *) calloc( 1, sizeof(char)*STRING_SIZE );
   Sleft = STRING_SIZE-1;

   /* reset symbol table */
   for ( i = 0; i < MAX_SYMBOLS; i++)
	SymTable[i] = NULL;
}

/* ssAddSymbol(s) - Adds the symbol s to the symbol table.  Returns the
	symbol table entry index.  Jumps to top-level on error.
*/
int ssAddSymbol(s)
char *s;
{
   int entry, retry, scomp;

   retry = entry = mcHash(s, MAX_SYMBOLS);

   /* find the appropriate bucket for this symbol */
   while ( (SymTable[retry] != NULL) &&
	   ((scomp = strcmp(SymTable[retry], s)) != 0) ) {

	retry = mcRehash( s, retry, MAX_SYMBOLS );

	/* we don't want to get into an infinite loop though */
	/* this line depends entirely on the rehash function! */
	if ( retry == entry ) break;
   }

   /* add the symbol if it's not already in the table */
   if ( SymTable[retry] == NULL ) {

	SYM_DEBUG("Adding a new symbol.");

	/* add the symbol to the table */
	SymTable[retry] = ssAddString(s);

	return retry;
   }

   /* maybe the symbol is already in the table? */
   if ( scomp == 0 ) {

	SYM_DEBUG("Linking to old symbol.");

	return retry;
   }

   /* couldn't find a slot for the symbol */
   RT_ERROR("Symbol table full!");

   /*NOTREACHED!*/
   /* please the compiler */
   return -1;
}

/* ssIsSymbol(s) - Returns TRUE if s is a symbol that is in the symbol
	table.
*/
int ssIsSymbol(s)
char *s;
{
   int entry, retry, scomp;

   retry = entry = mcHash(s, MAX_SYMBOLS );

   /* find the appropriate bucket for this symbol */
   while ( (SymTable[retry] != NULL) &&
	   ((scomp = strcmp(SymTable[retry], s)) != 0) ) {

	retry = mcRehash( s, retry, MAX_SYMBOLS );

	/* we don't want to get into an infinite loop though */
	/* this line depends entirely on the rehash function! */
	if ( retry == entry ) break;
   }

   if ( scomp == 0 )
	return TRUE;

   return FALSE;
}

/* ssAddString(s) - Adds string S to the string storage buffer.  Returns a
	pointer to the newly "allocated" string.
*/
char *ssAddString(s)
char *s;
{
   char *start;

   if ( Sleft <= 0 )
	RT_ERROR("Out of string space!");

   /* copy the string into the string space */
   for ( start = STRING_SPACE; *s != EOS && Sleft > 2; ++s, ++STRING_SPACE, --Sleft )
	*STRING_SPACE = *s;
   *STRING_SPACE++ = EOS; --Sleft;	/* string is EOS terminated */

   return start;
}

/* ssSubstring(s, beg, end) - Copies the substring defined by the indexes of
	string s beg and end into a newly allocated string.
*/
char *ssSubstring(s, beg, end)
char *s;
int beg, end;
{
   char *start, *stop;

   stop = s+end+1;
   s += beg;
   start = STRING_SPACE;

   for ( ; s != stop && Sleft > 2 && *s != EOS; ++s, ++STRING_SPACE, --Sleft )
	*STRING_SPACE = *s;
   *STRING_SPACE++ = EOS; --Sleft;

   return start;
}

/* ssAppend(s1, s2) - Returns a newly allocated string made from appending
	s2 to s1.
*/
char *ssAppend(s1, s2)
char *s1, *s2;
{
   char *start;

   start = STRING_SPACE;
   for ( ; *s1 != EOS && Sleft > 2 ; ++s1, ++STRING_SPACE, --Sleft )
	*STRING_SPACE = *s1;

   for ( ; *s2 != EOS && Sleft > 2 ; ++s2, ++STRING_SPACE, --Sleft )
	*STRING_SPACE = *s2;
   *STRING_SPACE++ = EOS; --Sleft;

   return start;
}
