/* the lexical scanner for Scheme

   Version 1

	* Having token_type as a global variable is a little trick to
	make PutBack() easy and fast.  GetToken() returns the token's
	type and the caller puts this type into token_type.  Having
	token_type static here would require me to explicity save it
	here.  It's easier this way (although potentially more dangerous).

   BUGS:

	- Needs better error handling and better checking.
*/

#include "machine.h"

#include "glo.h"
#include "debug.h"
#include "scanner.h"

/* the global variables */
char token[MAX_TOKEN];		/* the current token's string */
int inum;			/* token: integer value */
REAL_NUM fnum;			/* token: float value */
int token_type;			/* the current token's type */
static prev_token;		/* previous token */
static char *tknptr;		/* token pointer used in scanner */
static char in_line[MAX_LINE];	/* the current input line */
static int line_num;		/* the current line # */

/* private prototypes */
static int read_line( C_FILE C_PTR );
static void skip_space( C_VOID );
static char convert_char( C_CHAR C_PTR );

/* private macros */
#define islegal(a)	( ((a)) != EOS && !isspace((a)) && ((a)) != '(' && ((a)) != ')' && ((a)) != '[' && ((a)) != ']' )
#define TOUPPER(c)	( islower((c)) ? toupper((c)) : (c) )

/* InitScanner() - resets all of the scanner's variables. */
void InitScanner()
{
   token[0] = EOS;
   in_line[0] = EOS;
   token_type = NO_TOKEN;
   prev_token = NO_TOKEN;
   line_num = 0;
   tknptr = in_line;
}

/* GetToken() - returns the token type of the next token in the line.  it
	has a side-effect of putting the token in the variable token.
*/
int GetToken(f)
FILE *f;
{
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
   skip_space();

   /* may need next line or skip a comment */
   if ( *tknptr == EOS || *tknptr == CDELIM ) {
	if ( read_line(f) == EOF )
		return EOF;

	goto new_line;
   }

   /* check for parenthesis and brackets */
   if (*tknptr == '(') {
	*tptr++ = '('; *tptr = EOS;
	++tknptr;				/* advance ptr */
	return LP;
   }
   if (*tknptr == ')') {
	*tptr++ = ')'; *tptr = EOS;
	++tknptr;				/* advance ptr */
	return RP;
   }
   if (*tknptr == '[') {
	*tptr++ = '['; *tptr = EOS;
	++tknptr;				/* advance ptr */
	return LB;
   }
   if (*tknptr == ']') {
	*tptr++ = ']'; *tptr = EOS;
	++tknptr;				/* advance ptr */
	return RB;
   }

   /* check for '\'' */
   if ( *tknptr == '\'' ) {
	*tptr++ = '\''; *tptr = EOS;
	++tknptr;				/* advance ptr */
	return QUOTE;
   }

   /* check for '`' */
   if ( *tknptr == '`' ) {
	*tptr++ = '`'; *tptr = EOS;
	++tknptr;				/* advance ptr */
	return QUASI;
   }

   /* check for "#\" */
   if ( *tknptr == '#' && *(tknptr+1)=='\\' ) {
	tknptr += 2;
	if ( *(tknptr+1) && islegal( *(tknptr+1) ) ) {

		while ( islegal( *tknptr ) ) {
			*tptr++ = TOUPPER(*tknptr);
			++tknptr;
		}
		*tptr = EOS; tptr = token;
		*tptr++ = convert_char(token);

	} else *tptr++ = *tknptr++;		/* advance ptr */

	*tptr = EOS;
	return CHAR;
   }

   /* check for "#(" */
   if ( *tknptr == '#' && *(tknptr+1) == '(' ) {
	tknptr += 2;
	inum = UNKNOWN_ELEMS;
	return VECTOR;
   }

   /* check for "#<NUMBER>(" */
   if ( *tknptr == '#' ) {
	++tknptr;
	if ( !isdigit(*tknptr) )
		return NO_TOKEN;

	while ( isdigit(*tknptr) )
		*tptr++ = *tknptr++;
	*tptr = EOS;

	++tknptr;				/* advance pntr */

	inum = atoi(token);
	return VECTOR;
   }

   /* check for '"' */
   if ( *tknptr == '"' ) {
	/* strings are supposed to be able to span lines.  this
	 * should be fixed, but i'm too lazy right now.
	 */
	++tknptr;
	while ( *tknptr && *tknptr != '"' ) {
		if ( *tknptr == '\\' )
			++tknptr;

		*tptr++ = *tknptr++;
	}

	*tptr = EOS;
	++tknptr;				/* advance ptr */
	return STRING;
   }

   /* check for , and ,@ */
   if ( *tknptr == ',' ) {
	*tptr++ = ',';
	++tknptr;				/* advance ptr */
	if ( *tknptr == '@' ) {
		*tptr++ = '@'; *tptr = EOS;
		++tknptr;			/* advance ptr */
		return UNQ_SPLICE;
	}

	*tptr = EOS;
	return UNQUOTE;
   }

   /* check for DOT alone, if not alone fall into number */
   if ( *tknptr == '.' ) {
	integer = FALSE;
	*tptr++ = *tknptr++;			/* advance ptr */
	if ( isspace(*tknptr) ) {
		*tptr = EOS;
		return DOT;			/* ptr is advanced */
	}
   }

   /* check for operators, if not alone fall into number */
   if ( strchr("+-*/", *tknptr) ) {
	*tptr++ = *tknptr++;			/* advance ptr */
	if ( isspace(*tknptr) ) {
		*tptr = EOS;
		return SYMBOL;			/* ptr is advanced */
	}
   }

   /* check for numeric atom */
   if ( isdigit(*tknptr) ) {
	*tptr++ = *tknptr++;
	while ( isdigit(*tknptr) || *tknptr == '.' ) {
		if (*tknptr == '.') integer = FALSE;
		*tptr++ = *tknptr++;		/* ptr is advanced */
	}

	/* convert to binary format */
	*tptr = EOS;
	if (integer == TRUE) {
		inum = atoi(token);
		*token = EOS;
		return INT;
	}
	else {
		fnum = (REAL_NUM) atof(token);
		*token = EOS;
		return FLOAT;
	}
   }

   /* alpha-numeric atom now */
   while ( islegal(*tknptr) && *tknptr != CDELIM ) {
	*tptr = TOUPPER(*tknptr);
	++tptr ; ++tknptr;		/* ptr is advanced */
   }

   *tptr = EOS;
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

/* read_line() - Reads one line from the file stream f.  Returns 0 on success,
	EOF if EOF is encountered.
*/
static int read_line(f)
FILE *f;
{
   if ( fgets(in_line, MAX_LINE, f) == NULL )
	return EOF;

   tknptr = in_line;
   return 0;
}

/* skip_space() - skips the leading whitespace in the line */
static void skip_space()
{
   while ( isspace(*tknptr) ) {
	if (*tknptr == '\n') line_num++;
	++tknptr;
   }
}

/* convert_char(s) - S is a string denoting a character.  Converts and returns
	that character.  i.e. "space" returns space, "newline" returns '\n'.
	S is null terminated.
*/
static char convert_char(s)
char *s;
{
   if ( strcmp(s, "SPACE") == 0 )
	return ' ';

   return '\n';
}
