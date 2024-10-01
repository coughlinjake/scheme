/* mc_io.c - the C, input/output routines

   Version 1
	- See notes in micro.c.

   BUGS:
	- mcDumpCons() and mcRestCons() should be rewritten to recur on
	the car and iterate on the cdr to save stack space on large lists.
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

#define TEMP_SYMBOL_SIZE	500

/* local prototypes */
static CONS mcReadList( C_FILE C_PTR );
static CONS mcReadAtom( C_FILE C_PTR );
static void mcWriteAtom( C_CONS X C_FILE C_PTR X C_INT );
static CONS mcReadQuote( C_FILE C_PTR );
static CONS mcReadVector( C_FILE C_PTR );
static void mcDumpCons( C_CONS X C_FILE C_PTR );
static CONS mcRestCons( C_FILE C_PTR );

/* local macros */
#define IsNil(x)   ( strcmp((x), "#NULL") == 0 )
#define IsT(x)	   ( strcmp((x), "#T") == 0 )
#define IsF(x)     ( strcmp((x), "#F") == 0 )

/* ----------------------------------------------------------------------- */
/*                            Port Open/Close				   */
/* ----------------------------------------------------------------------- */

/* mcOpen(n) - Opens file named n, and returns a port. */
CONS mcOpen(n, m)
char *n, *m;
{
   CONS port;
   FILE *fp;

   if ( (fp = fopen(n, m)) == NULL ) {
	return NIL;
   }

   port = NewCons( PORT, 0, 0 );
   mcCpy_Port(port, fp);
   if ( strcmp(m, "r") == 0 )
	mcCpy_PortType(port, INPUT);
   else if ( strcmp(m, "w") == 0 )
	mcCpy_PortType(port, OUTPUT);
   return port;
}

/* mcClose(p) - Closes port p. */
void mcClose(p)
CONS p;
{
   if ( mcGet_PortType(p) != CLOSED ) {
	fclose( mcGet_Port(p) );
	mcCpy_PortType(p, CLOSED);
   }
}

/* ----------------------------------------------------------------------- */
/*                              Scheme READ				   */
/* ----------------------------------------------------------------------- */

/* mcRead(f) - Gets and returns the next Scheme object from stream f.  If
	it encounters a token it can't handle, it puts the token back and
	returns NULL.  Remember that NULL shouldn't occur anywhere in a
	list.
*/
CONS mcRead(f)
FILE *f;
{
   token_type = GetToken(f);

   switch ( token_type ) {
      case QUOTE:
      case QUASI:
      case UNQUOTE:
      case UNQ_SPLICE:
	PutBack();
	return mcReadQuote(f);

      case SYMBOL:
      case STRING:
      case INT:
      case FLOAT:
      case CHAR:
	PutBack();
	return mcReadAtom(f);

      case VECTOR:
	PutBack();
	return mcReadVector(f);

      case LP:
      case LB:
	PutBack();
	return mcReadList(f);

      case RP:
      case RB:
	PutBack();
	return NULL;

      case EOF:
	return EOF_OBJ;

      default:
	RT_ERROR("READ: Syntax error.");
	break;
   }

   return NIL;
}

/* mcReadList(f) */
static CONS mcReadList(f)
FILE *f;
{
   ENTER;

   REG(head);
   REG(tail);
   REG(elem);
   int end;

   token_type = GetToken(f);
   R(head) = R(tail) =  R(elem) = NIL;

   if ( token_type == LP )
	end = RP;
   else if ( token_type == LB )
	end = RB;
   else
	assert(0);

   token_type = GetToken(f);

   /* read the entire list */
   while ( token_type != end && token_type != DOT && token_type != NO_TOKEN && token_type != EOF ) {

	PutBack();

	/* get this element */
	R(elem) = mcRead(f);
	assert( R(elem) );

	R(elem) = mcCons( R(elem), NIL );	/* building lists here! */

	/* link the element into the current list */
	if ( R(head) == NIL ) {
		/* just starting the list */
		R(head) = R(elem);
		R(tail) = R(head);
	} else {
		/* adding to end of current list */
		mcSetCdr( R(tail), R(elem) );
		R(tail) = R(elem);
	}

	/* next element */
	token_type = GetToken(f);
   }

   /* quit on DP? cdr = next expression (and is end of list) */
   if ( token_type == DOT ) {

	R(elem) = mcRead(f);
	assert( R(elem) );

	/* insert the expression, NOTE: list must already exist! */
	if ( mcNull( R(head) ) ) {
		RT_ERROR("Misplaced dot!");
	}
	else
		mcSetCdr( R(tail), R(elem) );

	/* make sure we've read the end of the list */
	if ( (token_type = GetToken(f)) != end ) {

		/* skip the rest of the bogus list */
		token_type = GetToken(f);
		while ( token_type != end && token_type != NO_TOKEN && token_type != EOF )
			token_type = GetToken(f);

		WARNING("Misplaced dot.  Elements skipped.");
	}
   }
   else if ( token_type == NO_TOKEN ) {
	RT_ERROR("Unexpected end of input.");
   }

   MCLEAVE R(head);
}

/* mcReadAtom() - Gets the next atom in the input stream and returns it in
	a cons node.
	- Easy to handle keyword #NULL   => ()
	- Easy to handle keyword #T => T
	- Easy to handle keyword #F => F
	- Easy to handle keyword 'EXPR   => (QUOTE EXPR)
	- Easy to handle keyword `EXPR   => (QUASIQUOTE EXPR)
	- Easy to handle keyword ,EXPR   => (UNQUOTE EXPR)
	- Easy to handle keyword ,@ EXPR => (UNQUOTE-SPLICE EXPR)

	- MM Note: No need to declare elem as REG() because it's defined
	after possible GC happens.
*/
static CONS mcReadAtom(f)
FILE *f;
{
   CONS elem;

   token_type = GetToken(f);

   /* handle special symbols here */
   if ( token_type == SYMBOL ) {
	if ( IsNil( token ) )
		return NIL;

	if ( IsT( token ) )
		return T;

	if ( IsF( token ) )
		return F;
   }

   switch ( token_type ) {
     case FLOAT:
	elem = NewCons( FLOAT, 0, 0 );
	mcCpy_Float(elem, fnum);
	break;

     case INT:
	elem = NewCons( INT, 0, 0 );
	mcCpy_Int(elem, inum);
	break;

     case SYMBOL:
	elem = NewCons( SYMBOL, 0, 0 );
	mcCpy_Sym(elem, token);
	break;

     case CHAR:
	elem = NewCons( CHAR, 0, 0 );
	mcCpy_Char(elem, *token);
	break;

     case STRING:
	elem = NewCons( STRING, 0, 0 );
	mcCpy_Str(elem, token);
	break;

     default:
	printf("PARSING ERROR: Illegal token encountered: %s.", token);
	ERROR;
   }

   return elem;
}

/* mcReadQuote(f) */
static CONS mcReadQuote(f)
FILE *f;
{
   ENTER;
   REG(quote);
   REG(elem);

   token_type = GetToken(f);

   R(quote) = NewCons( SYMBOL, 0, 0 );
   switch ( token_type ) {

     case QUOTE:
	mcCpy_Sym( R(quote), "QUOTE");
	break;

     case QUASI:
	mcCpy_Sym( R(quote), "QUASIQUOTE");
	break;

     case UNQUOTE:
	mcCpy_Sym( R(quote), "UNQUOTE");
	break;

     case UNQ_SPLICE:
	mcCpy_Sym( R(quote), "UNQUOTE-SPLICE");
	break;

     default:
	assert(0);
   }

   R(elem) = mcRead(f);
   assert( R(elem) );

   R(elem) = mcCons( R(elem), NIL );
   R(quote) = mcCons( R(quote), R(elem) );

   MCLEAVE R(quote);
}

/* mcReadVector(f) */
static CONS mcReadVector(f)
FILE *f;
{
   ENTER;
   REG(v);

   token_type = GetToken(f);
   assert( token_type == VECTOR );

   if ( inum == UNKNOWN_ELEMS ) {
	/* unknown # of elements, read the elements into a list and then
	 * convert this list into a vector
	 */
	ENTER;
	REG(head);
	REG(tail);
	REG(elem);

	R(head) = R(tail) = R(elem) = NIL;

	token_type = GetToken(f);
	while ( token_type != RP && token_type != NO_TOKEN && token_type != EOF ) {

		PutBack();

		/* get this element */
		R(elem) = mcRead(f);
		assert( R(elem) );
		R(elem) = mcCons( R(elem), NIL );

		/* link the element into the current list */
		if ( R(head) == NIL ) {
			/* just starting the list */
			R(head) = R(elem);
			R(tail) = R(head);
		} else {
			/* adding to end of current list */
			mcSetCdr( R(tail), R(elem) );
			R(tail) = R(elem);
		}

		/* next element */
		token_type = GetToken(f);
	}

	R(v) = mcLstVector( R(head) );

   } else {
	/* user specified the # of elements, allocate a vector big enough
	 * and read the elements directly into the vector.
	 */
	int cnt;

	if ( inum < 0 ) {
		RT_ERROR("READ: Negative vector size specified for vector.");
	}

	R(v) = mcMakeVector( inum, NIL );

	for ( cnt = 0; cnt < inum; ++cnt ) {
		*mcVect_Ref(R(v), cnt) = mcRead(f);
		if ( *mcVect_Ref( R(v),cnt ) == NULL ) {
			*mcVect_Ref( R(v), cnt ) = NIL;
			break;
		}
	}

	token_type = GetToken(f);
	if ( token_type != RP ) {
		RT_ERROR("READ: Vector syntax error.");
	}
   }

   MCLEAVE( R(v) );
}

/* ----------------------------------------------------------------------- */
/*                             Scheme WRITE				   */
/* ----------------------------------------------------------------------- */

/* mcWriteAtom(c) - Writes the atom c to the file stream f. */
static void mcWriteAtom(c, f, d)
CONS c;
FILE *f;
int d;
{
   /* '() is an atom */
   if ( mcNull(c) ) {
	fprintf(f, "()");
	return;
   }

   switch ( mcKind(c) ) {
	case TOBJ:
		fprintf(f, "#T");
		break;

	case FOBJ:
		fprintf(f, "#F");
		break;

	case EOFOBJ:
		fprintf(f, "#EOF");
		break;

	case SYMBOL:
		fprintf(f, "%s", mcGet_Sym(c));
		break;

	case INT:
		fprintf(f, "%d", mcGet_Int(c));
		break;

	case FLOAT:
		fprintf(f, REAL_FORMAT, mcGet_Float(c));
		break;

	case STRING:
		if ( d )
			fprintf(f, "%s", mcGet_Str(c));
		else fprintf(f, "\"%s\"", mcGet_Str(c));
		break;

	case CHAR:
		if ( d )
			fprintf(f, "%c", mcGet_Char(c));
		else {
			if ( mcGet_Char(c) == '\n' )
				fprintf(f, "#\\newline");
			else if ( mcGet_Char(c) == ' ' )
				fprintf(f, "#\\space");
			else fprintf(f, "#\\%c", mcGet_Char(c));
		}
		break;

	case PORT:
		fprintf(f, "#<PORT,%p>", mcGet_Port(c));
		break;

	case CFUNC:
		fprintf(f, "#<Primitive procedure %s>", mcPrim_Name(c));
		break;

	case CFORM:
		fprintf(f, "#<Primitive form %s>", mcPrim_Name(c));
		break;

	case FORM:
		fprintf(f, "#<Form>");
		break;

	case BCODES:
		fprintf(f, "#<Code,%d>", mcBC_CSize(c));
		break;

	case EXEPOINT:
		fprintf(f, "#<PC,%d>", mcExe_PC(c));
		break;

	case CLOSURE:
		fprintf(f, "#<Closure>");
		break;

	case CONT:
		fprintf(f, "#<Continuation>");
		break;

	case VECTOR:
	      {
		int cnt;

		fprintf(f, "#(");

		cnt = 0;
		if ( cnt < mcVect_Size(c) ) {
			mcWrite( *mcVect_Ref(c, cnt), f );
			for ( cnt = 1; cnt < mcVect_Size(c); ++cnt ) {
				fprintf(f, " ");
				mcWrite( *mcVect_Ref(c, cnt), f );
			}
		}

		fprintf(f, ")");
	      }
		break;

	case RESUME:
		fprintf(f, "#<Resume, %d>", mcGet_Int(c));
		break;

	default:
		assert(0);
   }

   fflush(f);
}

/* mcEmit(c, f) - Writes the Scheme object c to the file stream f.  Used
	to be called mcWrite() except that I didn't want the definition of
	mcWrite() to change when I added (DISPLAY obj).  mcWrite(l,p) is a
	macro expanding to: mcEmit(l, p, FALSE).  mcDisplay(l,p) is a macro
	expanding to mcEmit(l,p, TRUE).
*/
void mcEmit(c, f, d)
CONS c;
FILE *f;
int d;
{
   /* NULL is illegal in the list data-type! */
   assert( c != NULL );

   /* outputting an atom */
   if ( mcAtom(c) ) {
	mcWriteAtom(c, f, d);
	return;
   }

   /* beginning of a list */
   fprintf(f, "(");

   /* walk thru list */
   while ( !mcNull(c) ) {

	mcEmit( mcGet_Car(c), f, d );
	c = mcGet_Cdr(c);

	/* improper list? */
	if ( !mcNull(c) && !mcPair(c) ) {
		fprintf(f, " . ");
		mcWriteAtom(c, f, d);
		break;
	}

	if ( !mcNull(c) )
		fprintf(f, " ");
   }

   fprintf(f, ")");
}

/* ----------------------------------------------------------------------- */
/*                   Load a file of Scheme Expressions			   */
/* ----------------------------------------------------------------------- */

/* mcLoad(name) - Reads expressions sequentially from the file named name,
	and evaluates them (throwing away the result).  Returns TRUE if
	a file was loaded, FALSE if the file wasn't found.
*/
int mcLoad(name)
char *name;
{
   CONS port;

   /* open the file and put into a port */
   port = mcOpen(name, "r");
   if ( mcNull(port) )
	return FALSE;

   /* push the port on the value stack */
   mcPushVal( port );

   /* push a dummy value on the val stack for mcResLoad() to pop off
    * and throw away.
    */
   mcPushVal( T );

   return mcResLoad( evMkResume(prLoad) );
}

/* mcResLoad(res) - Resume loading a file. */
int mcResLoad(resume)
CONS resume;
{
   ENTER;
   REG(port);
   REG(exp);
   REG(res);

   /* save the resume since it's not guaranteed to be held */
   R(res) = resume;

   /* throw away last evaluation */
   mcPopVal();

   R(port) = mcPopVal();

   /* read the next expr */
   if ( (R(exp) = mcRead( mcGet_Port(R(port)) )) == EOF_OBJ ) {
	mcClose( R(port) );
	MCLEAVE TRUE;
   }

   mcPushVal( R(port) );
   mcPushExpr( R(res) );
   mcPushExpr( R(exp) );
   MCLEAVE TRUE;
}

/* ----------------------------------------------------------------------- */
/*                           Dump Environment				   */
/* ----------------------------------------------------------------------- */

/* mcDumpEnv(f) - Dump the current environment to file f.  Returns TRUE if
	successful, FALSE if failed.
*/
int mcDumpEnv(f)
FILE *f;
{
   CONS val;
   int sym, len;

   for ( sym = 0; sym < MAX_SYMBOLS; ++sym ) {
	/* don't dump:
	 *	o NULL bindings
	 *	o CFORMS and CFUNCS
	 */
	val = *mcVect_Ref( mcGet_Global(glo_env), sym );
	if ( val == NULL || mcFunc(val) || mcForm(val) )
		continue;

	/* dump the symbol */
	len = strlen( SymTable[sym] );
	fwrite( &len, sizeof(int), 1, f );
	fwrite( SymTable[sym], sizeof(char), len, f );

	/* dump it's binding */
	mcDumpCons( val, f );
   }

   return TRUE;
}

/* mcDumpCons(c) - Dumps the cons c to the file stream f. */
static void mcDumpCons(c, f)
CONS c;
FILE *f;
{
   /* write out the scheme type */
   fwrite( &mcKind(c), sizeof(int), 1, f );

   /* write out the data */
   switch ( mcKind(c) ) {
	case NILNODE:
	case NULLNODE:
	case TOBJ:
	case FOBJ:
	case EOFOBJ:
		/* nothing needs to be output */
		return;

	case SYMBOL:
	{
		int len;

		/* write length of symbol */
		len = strlen( mcGet_Sym(c) );
		fwrite( &len, sizeof(int), 1, f );

		/* write symbol */
		fwrite( mcGet_Sym(c), sizeof(char), len, f );
		return;
	}

	case INT:
		fwrite( &mcGet_Int(c), sizeof(int), 1, f );
		return;

	case FLOAT:
		fwrite( &mcGet_Float(c), sizeof(REAL_NUM), 1, f );
		return;

	case STRING:
	{
		int len;

		/* write length of symbol */
		len = strlen( mcGet_Str(c) );
		fwrite( &len, sizeof(int), 1, f );

		/* write symbol */
		fwrite( mcGet_Str(c), sizeof(char), len, f );
		return;
	}

	case CHAR:
		fwrite( &mcGet_Char(c), sizeof(char), 1, f );
		return;

	case BCODES:
	{
		int cst;

		/* write code and cnst sizes */
		fwrite( &mcBC_CSize(c), sizeof(int), 1, f );
		fwrite( &mcBC_CCSize(c), sizeof(int), 1, f );

		/* write the code and cnsts */
		fwrite( mcBC_Code(c), sizeof(char), mcBC_CSize(c), f );
		for ( cst = 0; cst < mcBC_CCSize(c); ++cst )
			mcDumpCons( *(mcBC_Const(c)+cst), f);

		return;
	}

	case CLOSURE:
		mcDumpCons( mcCl_Env(c), f );
		mcDumpCons( mcCl_Parms(c), f );
		mcDumpCons( mcCl_Body(c), f );
		return;

	case VECTOR:
	{
		int cnt;

		fwrite( &mcVect_Size(c), sizeof(int), 1, f );
		for ( cnt = 0; cnt < mcVect_Size(c); ++cnt )
			mcDumpCons( *mcVect_Ref(c, cnt), f);
		return;
	}

	case PAIR:
		mcDumpCons( mcGet_Car(c), f );
		mcDumpCons( mcGet_Cdr(c), f );
		return;

	default:
		/* unsupported Scheme data-types */
		printf("mcDumpCons(): Unsupported data-type %d.\n", mcKind(c));
		assert(0);
		break;
   }
}

/* ----------------------------------------------------------------------- */
/*                              Restore Env				   */
/* ----------------------------------------------------------------------- */

/* mcRestEnv(f) - Restore the previous environment from file f.  Returns TRUE
	if successful, FALSE if failed.
*/
int mcRestEnv(f)
FILE *f;
{
   ENTER;
   REG(sym);
   REG(val);
   char *tsym;
   int len;

   /* allocate a temporary buffer to hold the symbol we're going to bind
    * to.  we allocate buffer off heap to save the stack (Scheme symbols
    * aren't really supposed to have a size limit).
    */
   if ( (tsym = (char *)malloc( TEMP_SYMBOL_SIZE )) == NULL ) {
	printf("Can't restore environment, not enough memory for temporary\n");
	printf("symbol.\n");
	return FALSE;
   }

   R(sym) = NewCons( SYMBOL, 0, 0 );

   while ( TRUE ) {

	/* restore the symbol */
	if ( fread( &len, sizeof(int), 1, f ) < 1 )
		/* didn't read the length => EOF */
		break;
	fread( tsym, sizeof(char), len, f );
	*(tsym+len) = EOS;
	mcCpy_Sym( R(sym), tsym );

	/* restore the value */
	R(val) = mcRestCons( f );

	/* bind */
	evDefGlobal( R(sym), R(val) );
   }

   free( tsym );

   MCLEAVE TRUE;
}

/* mcRestCons(c) - Restore the next CONS from stream f and return it. */
static CONS mcRestCons(f)
FILE *f;
{
   ENTER;
   REG(c);
   int kind, len;
   char *tsym;

   if ( (tsym = (char *)malloc( TEMP_SYMBOL_SIZE )) == NULL ) {
	printf("mcRestCons(): Can't allocate temporary symbol.\n");
	return NIL;
   }

   /* read the node type */
   fread( &kind, sizeof(int), 1, f );

   /* read in the data */
   switch ( kind ) {
	case NILNODE:
		R(c) = NIL;
		break;

	case TOBJ:
		R(c) = T;
		break;

	case FOBJ:
		R(c) = F;
		break;

	case EOFOBJ:
		R(c) = EOF_OBJ;
		break;

	case SYMBOL:
		R(c) = NewCons( SYMBOL, 0, 0 );

		/* read the symbol */
		fread( &len, sizeof(int), 1, f );
		fread( tsym, sizeof(char), len, f );
		*(tsym+len) = EOS;

		/* copy the symbol into the CONS node */
		mcCpy_Sym( R(c), tsym );
		break;

	case INT:
		R(c) = NewCons( INT, 0, 0 );
		fread( &mcGet_Int(R(c)), sizeof(int), 1, f );
		break;

	case FLOAT:
		R(c) = NewCons( FLOAT, 0, 0 );
		fread( &mcGet_Float(R(c)), sizeof(REAL_NUM), 1, f );
		break;

	case STRING:
		R(c) = NewCons( STRING, 0, 0 );

		/* read the symbol */
		fread( &len, sizeof(int), 1, f );
		fread( tsym, sizeof(char), len, f );
		*(tsym+len) = EOS;

		/* copy the symbol into the CONS node */
		mcCpy_Str( R(c), tsym );
		break;

	case CHAR:
		R(c) = NewCons( CHAR, 0, 0 );
		fread( &mcGet_Char(R(c)), sizeof(char), 1, f );
		break;

	case BCODES:
	{
		int cst, cst2;

		/* read code and cnst sizes */
		fread( &len, sizeof(int), 1, f );
		fread( &cst, sizeof(int), 1, f );

		R(c) = NewCons( BCODES, len, cst );

		/* read the code */
		fread( mcBC_Code(R(c)), sizeof(char), len, f );

		/* read the constants */
		for ( cst2 = 0; cst2 < cst; ++cst2 )
			*(mcBC_Const(R(c))+cst2) = mcRestCons(f);

	}
		break;

	case CLOSURE:
		R(c) = NewCons( CLOSURE, 0, 0 );
		mcCl_Env( R(c) ) = mcRestCons(f);
		mcCl_Parms( R(c) ) = mcRestCons(f);
		mcCl_Body( R(c) ) = mcRestCons(f);
		break;

	case VECTOR:
	{
		int cnt;

		fread( &len, sizeof(int), 1, f );
		R(c) = NewCons( VECTOR, len, 0 );

		for ( cnt = 0; cnt < len; ++cnt )
			*mcVect_Ref( R(c), cnt) = mcRestCons(f);
	}
		break;

	case PAIR:
		R(c) = NewCons( PAIR, 0, 0 );
		mcGet_Car( R(c) ) = mcRestCons(f);
		mcGet_Cdr( R(c) ) = mcRestCons(f);
		break;

	default:
		/* unsupported Scheme data-types */
		printf("mcRestCons(): Unsupported data-type %d.\n", kind);
		assert(0);
		break;
   }

   free( tsym );
   MCLEAVE R(c);
}
