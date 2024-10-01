/* scanner.c -- the lexical scanner for Scheme

   Version 2 (jk0) Operates on characters.

	* MAX_TOKEN is HUGE to account for really long strings.  By far
	the hardest bugs to find in the interpeter have been when MAX_TOKEN
	wasn't long enough to hold the current token.  Why are ``strings''
	in C so god-damn lame?!?!

	* Operates on characters forcing the OS to handle line-buffering.
	This is fine for line-buffering OS's but might not be so hot for
	OS's without buffering.

	* The best solution is probably to add a table of line buffers,
	and we switch to the correct line buffer for the file that was
	passed to the scanner.

	* Having token_type as a global variable is a little trick to
	make PutBack() easy and fast.  GetToken() returns the token's
	type and the caller puts this type into token_type.  Having
	token_type static here would require me to explicity save it
	here.  It's easier this way (although potentially more dangerous).

   Version 1 (jk0) Operates on lines.

	* Mega bug in operating on lines.  Since I only have 1 line buffer,
	if I scan from file A and fill the line buffer with info from file
	A then scan from file B before finishing the line for A, B is
	actually scanning A's line.  This wasn't a problem for interaction
	with the user, but caused havoc when used with Scheme's LOAD
	function on files with nested LOAD's ( (LOAD "A") but A contains
	the line (LOAD "B") ).

   BUGS:

	- Needs better error handling and better checking.
*/

#include "machine.h"

#include <ctype.h>
#include STRING_H
#include STDLIB_H

#include "glo.h"
#include "debug.h"
#include "scanner.h"

#define MAX_TOKEN	500

/* the global variables */
char token[MAX_TOKEN];		/* the current token's string */
int inum;			/* token: integer value */
REAL_NUM fnum;			/* token: float value */
int token_type;			/* the current token's type */
static prev_token;		/* previous token */

/* private prototypes */
static int get_char( C_FILE C_PTR );
static void unget_char( C_FILE C_PTR X C_INT );
static void flush_line( C_FILE C_PTR );
static char convert_char( C_CHAR C_PTR );

/* private macros */
#define islegal(a)	( ((a)) != EOS && !isspace((a)) && ((a)) != '(' && ((a)) != ')' && ((a)) != '[' && ((a)) != ']' )
#define TOUPPER(c)	( islower((c)) ? toupper((c)) : (c) )

/* InitScanner() - resets all of the scanner's variables. */
void InitScanner()
{
   token[0] = EOS;
   token_type = NO_TOKEN;
   prev_token = NO_TOKEN;
}

/* GetToken() - returns the token type of the next token in the line.  it
	has a side-effect of putting the token in the variable token.
*/
int GetToken(f)
FILE *f;
{
   int ch;		/* current char */
   char *tptr;		/* placement in token[] */
   BOOL integer;	/* true if the atom is an integer */

   /* see if we're going to hand back a previous token */
   if ( prev_token != NO_TOKEN ) {
	token_type = prev_token;
	prev_token = NO_TOKEN;
	return token_type;
   }

   /* reset variables */
   tptr = token;
   integer = TRUE;

new_line:
   /* skip leading whitespace */
   ch = get_char(f);
   while ( isspace(ch) )
	ch = get_char(f);

   /* read EOF? */
   if ( ch == EOF )
	return EOF;

   /* skip a comment */
   if ( ch == CDELIM ) {
	flush_line(f);
	goto new_line;
   }

   /* check for parenthesis and brackets */
   if ( ch == '(') {
	*tptr++ = '('; *tptr = EOS;
	return LP;
   }
   if ( ch == ')') {
	*tptr++ = ')'; *tptr = EOS;
	return RP;
   }
   if ( ch == '[') {
	*tptr++ = '['; *tptr = EOS;
	return LB;
   }
   if ( ch == ']') {
	*tptr++ = ']'; *tptr = EOS;
	return RB;
   }

   /* check for '\'' */
   if ( ch == '\'' ) {
	*tptr++ = '\''; *tptr = EOS;
	return QUOTE;
   }

   /* check for '`' */
   if ( ch == '`' ) {
	*tptr++ = '`'; *tptr = EOS;
	return QUASI;
   }

   /* check for "#" */
   if ( ch == '#' ) {
	/* save the input since it might not be a vector or a character */
	*tptr++ = ch;

	ch = get_char(f);
	if ( ch == '\\' ) {
		/* reading a character; reset tptr to start of token
		 * and read the next character and stick in token.
		 */
		tptr = token;
		ch = get_char(f);
		*tptr++ = (char)ch;

		/* if there's more, put that into token as well.
		 * #\newline and #\space are characters.
		 */
		ch = get_char(f);
		while ( islegal(ch) ) {
			*tptr++ = (char)ch;
			ch = get_char(f);
		}
		*tptr = EOS;

		/* read too much so put last char back */
		unget_char(f, ch);

		/* convert char */
		tptr = token;
		*tptr = convert_char(token);
		*++tptr = EOS;
		return CHAR;
	}
	else if ( ch == '(' ) {
		inum = UNKNOWN_ELEMS;
		return VECTOR;
	}
	else if ( isdigit(ch) ) {
		while ( isdigit(ch) ) {
			*tptr++ = (char)ch;
			ch = get_char(f);
		}
		*tptr = EOS;

		inum = atoi(token);
		unget_char(f, ch);
		return VECTOR;
	}
	else
		/* not reading a vector or char, must be reading a
		 * symbol
		 */
		goto read_symbol;
   }

   /* check for '"' */
   if ( ch == '"' ) {
	ch = get_char(f);
	while ( ch != EOF && ch != '"' ) {
		if ( ch == '\\' )
			ch = get_char(f);

		*tptr++ = (char)ch;

		ch = get_char(f);
	}

	*tptr = EOS;
	return STRING;
   }

   /* check for , and ,@ */
   if ( ch == ',' ) {
	*tptr++ = ',';
	ch = get_char(f);
	if ( ch == '@' ) {
		*tptr++ = '@'; *tptr = EOS;
		return UNQ_SPLICE;
	}

	*tptr = EOS;
	unget_char(f, ch);
	return UNQUOTE;
   }

   /* check for DOT alone, if not alone fall into number */
   if ( ch == '.' ) {
	integer = FALSE;
	*tptr++ = (char)ch;
	ch = get_char(f);
	if ( isspace(ch) ) {
		*tptr = EOS;
		return DOT;
	}
   }

   /* check for operators, if not alone fall into number */
   if ( strchr("+-*/", ch) ) {
	*tptr++ = (char)ch;
	ch = get_char(f);
	if ( isspace(ch) ) {
		*tptr = EOS;
		return SYMBOL;			/* ptr is advanced */
	}
   }

   /* check for numeric atom */
   if ( isdigit(ch) ) {
	*tptr++ = (char)ch;
	ch = get_char(f);
	while ( isdigit(ch) || ch == '.' ) {
		if (ch == '.') integer = FALSE;
		*tptr++ = (char)ch;
		ch = get_char(f);
	}

	/* convert to binary format */
	*tptr = EOS;
	if (integer == TRUE) {
		inum = atoi(token);
		*token = EOS;
		unget_char(f, ch);
		return INT;
	}
	else {
		fnum = (REAL_NUM) atof(token);
		*token = EOS;
		unget_char(f, ch);
		return FLOAT;
	}
   }

read_symbol:
   /* alpha-numeric atom now */
   while ( islegal(ch) && ch != CDELIM ) {
	*tptr = TOUPPER(ch);
	++tptr;
	ch = get_char(f);
   }

   *tptr = EOS;
   unget_char(f, ch);
   return SYMBOL;
}

/* PutBack() - Puts the current token back into the input stream. */
void PutBack()
{
   prev_token = token_type;
}

/* ----------------------------------------------------------------------- */
/*                      Low level Scanner routines			   */
/* ----------------------------------------------------------------------- */

/* get_char(f) - Get the next character from f. */
static int get_char(f)
FILE *f;
{
   return fgetc(f);
}

/* unget_char(f, c) */
static void unget_char(f, c)
FILE *f;
int c;
{
   ungetc(c, f);
}

/* flush_line(f) - Read and throw away everything until the next newline. */
static void flush_line(f)
FILE *f;
{
   int ch;

   while ( (ch = get_char(f)) != '\n' && ch != EOF )
	;
}

/* convert_char(s) - S is a string denoting a character.  Converts and returns
	that character.  i.e. "space" returns space, "newline" returns '\n'.
	S is null terminated.
*/
static char convert_char(s)
char *s;
{
   if ( strcmp(s, "space") == 0 )
	return ' ';
   else if ( strcmp(s, "newline") == 0 )
	return '\n';

   return *s;
}
