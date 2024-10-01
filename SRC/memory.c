/* the memory manager - Ver 2 */

/* MM notes:

	- When collecting byte-code, the code and the constant table must
	be explicity freed since they were explicity alloced.

	- When marking an execution point, the byte-code node associated
	with the execution point must be marked too.

	- Memory is allocated in a linked list of segments of size
	ALLOC_CONS (number of cons nodes to allocate).

	- Calls to new_cons() activate the GC whenever the free-list is
	empty.  If the GC fails to collect any cons nodes, system memory
	is obtained (malloc).

	- new_cons() sets the new cons node's mark to false.  GC (1) marks
	all reachable cons nodes with mark = TRUE, including the stack and
	the environment; (2) steps thru segments gathering all nodes whose
	mark = FALSE and sets all nodes with mark = TRUE to mark = FALSE.

		- local cons nodes must be of type register so that a GC
		doesn't steal the nodes invisiblely.  register variables
		are placed on the stack.

	- free_cons is a free-list linked by the cdr.  use the macro:
	next_free(c).

	- segment size is 850 cons nodes, roughly 10k on a PC
*/

#include "machine.h"

#ifdef AllocH
#	include <alloc.h>
#endif

#include STDLIB_H
#include MEMORY_H

#include "glo.h"
#include "symstr.h"
#include "error.h"
#include "micro.h"
#include "memory.h"

/* macros */
#define next_free(c)		mcGet_Cdr(c)	/* next node on free-list */
#define mark(p)			((p) -> mmark)	/* set mark */

/* global definitions */
int torture = FALSE;
int gc_debug = FALSE;

/* private definitions */
#define ALLOC_CONS	500	/* # of cons nodes to allocate for malloc */
#define INIT_SEGS	1	/* initial # of segments */

/* private structures */
struct Seg {
   struct Seg *next;
   CONS first;
} ;
typedef struct Seg SEGMENT;

/* private variables */
static CONS free_cons;			/* free list for cons nodes */
static SEGMENT *store;			/* pointer to first SEG */

/* private prototypes */
static BOOL add_segment( C_VOID );
static void old_cons( C_CONS );
static void mrkatom( C_CONS );
static void mrklist( C_CONS );
static void mark_all( C_VOID );
static BOOL GC( C_VOID );
static int cnt_free( C_VOID );

/* init_mem() - setup MM.  Initialize globals and store. */
void InitMem(argc, argv)
int argc;
char *argv[];
{
   int l;

   gc_debug = FALSE;
   torture = FALSE;

   /* command line arguments */
   for ( l = 0; l < argc; l++ ) {
	if ( argv[l][0] == '-' || argv[l][0] == '/' ) {
		switch ( argv[l][1] ) {
		   case 'g':
			gc_debug = TRUE;
			break;

		   case 't':
			torture = TRUE;
			break;
		}
	}
   }

   /* no free-list yet */
   free_cons = NULL;

   /* no store yet either */
   store = NULL;
}

/* GetMem() - Get initial memory. */
void GetMem()
{
   int l;

   /* get initial memory */
   for (l = 1; l <= INIT_SEGS; l++) {
	if ( (add_segment()) == FALSE ) {
		FATAL("MM ERROR in get_memory:  Not enough initial memory.");
	}
   }
}

/* NewCons(type, size, csize) - returns a new cons node with specified type.
	some types require additional memory of specified size.
*/
CONS NewCons(type, size, csize)
int type, size, csize;
{
   CONS temp;		/* safe because GC will come before allocation */

   /* the new cons node is the first in line from the free list.
    * if the free list is empty, a new segment	is allocated.
    * if we fail again, the program is terminated.
    */
   if ( torture ) {
	/* TORTURE: GC before EVERY allocation! */
	if ( GC() != TRUE ) {
		if ( add_segment() != TRUE )
			FATAL("MM ERROR WITH TORTURE: No memory left.\n");
	}
   } else {
	/* do we need memory? (1) GC (2) all nodes in use so get more */
	if ( free_cons == NULL ) {
		GC_DEBUG( "\nGCing\n" );
		if ( GC() != TRUE ) {
			GC_DEBUG( "\nGC failed.  adding segment!\n" );
			if ( add_segment() != TRUE )
				FATAL("MM ERROR in new_cons: No more memory.\n");
		}
	}
   }

   /* we better have memory by now! */
   assert( free_cons != NULL );

   /* take first cons node from free_cons */
   temp = free_cons;
   free_cons = next_free(free_cons);

   assert( mcKind(temp) == FREE );	/* just to make sure */

   mark(temp) = FALSE;
   mcKind(temp) = type;

   /* reset pntrs so that a garbage-collection won't send us off
    * into never-never land.
    */
   switch ( type ) {
      case VECTOR:
	mcVect_Size(temp) = size;
	if ( (mcGet_Vector(temp) = (CONS *)malloc( size * sizeof(CONS) )) == NULL ) {
		RT_ERROR("Out of memory; can't allocate vector.");
	}

	mcVectorFill( temp, NIL );
	break;

      case BCODES:
      {
	int cst;

	if ( (mcBC_Code(temp) = (unsigned char *)malloc( size )) == NULL ) {
		RT_ERROR("Out of memory; can't allocate byte-code.");
	}
	if ( (mcBC_Const(temp)= (CONS *)malloc( csize * sizeof(CONS) )) == NULL ) {
		RT_ERROR("Out of memory; can't allocate constant table.");
	}

	/* set the code and constant table sizes */
	mcBC_CSize(temp) = size;
	mcBC_CCSize(temp) = csize;

	for ( cst = 0; cst < csize; ++cst )
		*(mcBC_Const(temp)+cst) = NIL;
      }
	break;

      case CLOSURE:
	mcCl_Env(temp) = NIL;
	mcCl_Parms(temp) = NIL;
	mcCl_Body(temp) = NIL;
	break;

      case CONT:
	mcCont_Env(temp) = NIL;
	mcCont_Val(temp) = NIL;
	mcCont_Fnc(temp) = NIL;
	mcCont_Exp(temp) = NIL;
	break;

      case ENVMNT:
	mcGet_Nested(temp) = NIL;
	mcGet_Global(temp) = NIL;
	break;

      case PAIR:
	mcGet_Car(temp) = NIL;
	mcGet_Cdr(temp) = NIL;
	break;

      case EXEPOINT:
	mcExe_BC(temp) = NIL;
	mcExe_Env(temp) = NIL;
	break;

      default:
	break;
   }

   return temp;
}

/* ----------------------------------------------------------------------- */
/*                 Low level Memory Management Routines			   */
/* ----------------------------------------------------------------------- */

/* add_segment() - Adds a segment of new cons nodes.  The cons nodes are
	placed on the free_list.  Returns TRUE if successful, FALSE if
	failed.
*/
static BOOL add_segment()
{
   SEGMENT *new;
   CONS temp;		/* garbage collection not possible right now */
   int i;

   /* allocate segment then allocate cons nodes */
   if ( (new = (SEGMENT *) malloc( sizeof(SEGMENT) )) != NULL &&
	((new->first) = (CONS) malloc( sizeof(CONSNODE) * ALLOC_CONS )) != NULL) {

	/* we got our SEGMENT, add it to store */
	new -> next = store;
	store = new;

	/* carve up cons nodes and put them on the free-list */
	for (temp = (new->first), i = 1; i <= ALLOC_CONS ; i++, temp++) {
		/* set the kind of this node to free because old_cons()
		 * checks to see if the cons node is code.  at this point,
		 * the cons node has no type -- just left over junk in
		 * memory.
		 */
		mcKind(temp) = FREE;
		old_cons(temp);
	}

	return TRUE;

   } /* else return FALSE; */

   return FALSE;
}

/* old_cons(c) - Takes a cons node c and puts it on the free list. */
static void old_cons(c)
CONS c;
{
   switch ( mcKind(c) ) {
      case VECTOR:
	free( mcGet_Vector(c) );
	break;

      case BCODES:
	free( mcBC_Code(c) );
	free( mcBC_Const(c) );
	break;

      default:
	break;
   }

   /* clear all information in the CONS node to help farret out dangling
    * pointer problems.
    */
   memset( c, 0, sizeof(CONSNODE) );

   /* node nolonger in use */
   mcKind(c) = FREE;

   /* add node c to the front of the free list */
   next_free(c) = free_cons;
   free_cons = c;
}

/* mrkatom(a) - Marks this atom. */
static void mrkatom(a)
CONS a;
{
   assert( a != NULL );

   if ( mark(a) == TRUE )
	return;

   mark(a) = TRUE;

   /* '() is an atom */
   if ( mcNull(a) )
	return;

   /* this switch statement isn't really needed -- it's for my own
    * protection.  there are a lot of "atom" nodes.  if i forget to
    * handle one, this marker will terminate.  without the switch, if
    * i forget to handle one, the atom isn't marked and disappears right
    * out from underneath me.
    */
   switch ( mcKind(a) ) {
	case SYMBOL:
	case INT:
	case FLOAT:
	case STRING:
	case CHAR:
	case PORT:
	case CFUNC:
	case CFORM:
	case RESUME:
	case TOBJ:
	case FOBJ:
	case EOFOBJ:
		/* nothing to do for these data-types */
		break;

	case FORM:
		/* have to mark the form's information */
		mrklist( mcForm_Parms(a) );
		mrklist( mcForm_Body(a) );
		break;

	case CLOSURE:
		/* have to mark the closure's information */
		mrklist( mcCl_Parms(a) );
		mrklist( mcCl_Body(a) );
		mrklist( mcCl_Env(a) );
		break;

	case CONT:
		/* have to mark the continuation's info */
		mrklist( mcCont_Env(a) );
		mrklist( mcCont_Val(a) );
		mrklist( mcCont_Fnc(a) );
		mrklist( mcCont_Exp(a) );
		break;

	case BCODES:
	   {
		/* byte-code: have to mark the constant table */
		int l;

		for ( l = 0; l < mcBC_CCSize(a); ++l )
			mrklist( *(mcBC_Const(a)+l) );

	   }
		break;

	case VECTOR:
	   {
		int l;

		for ( l = 0; l < mcVect_Size(a); l++ )
			/* vectors can have NULL's since we use vectors
			 * to implement environments.
			 */
			if ( *mcVect_Ref(a, l) )
				mrklist( *mcVect_Ref(a, l) );
	   }

		break;

	case ENVMNT:
		/* mark the nested bindings and the global bindings */
		mrklist( mcGet_Nested(a) );
		mrkatom( mcGet_Global(a) );
		break;

	case EXEPOINT:
		/* execution point: have to mark the byte-code. */
		mrkatom( mcExe_BC(a) );

		/* have to mark the environment */
		mrklist( mcExe_Env(a) );
		break;

	default:
		/* woops, forgot to handle an atom data-type.  terminate. */
		printf("\nForgot %d.\n", mcKind(a));
		assert(0);
   }
}

/* mrklist( l ) - Marks all of the nodes in the list l. */
static void mrklist(l)
CONS l;
{
   assert( l != NULL );
   assert( mcKind(l) != FREE );			/* should NEVER happen! */

   /* already done this list? */
   if ( mark(l) == TRUE )
	return;

   /* l is an atom */
   if ( !mcPair(l) ) {
	mrkatom(l);
	return;
   }

   /* mark the list */
   while ( !mcNull(l) ) {

	mark(l) = TRUE;

	/* recur on CAR */
	mrklist( mcCar(l) );

	/* iterate on CDR */
	l = mcCdr(l);

	/* handle DP's */
	if ( !mcPair(l) ) {
		mrkatom(l);
		return;
	}
   }
}

/* mark_all() - Marks all of the nodes in use.
	NOTE:  NULL is used in environments to denote symbols which haven't
	been bound.  These need to be caught before mrklist() since NULL
	is totally illegal in any list.
*/
static void mark_all()
{
   CONS *i;

   /* mark the register stack */
   for (i = Top_RegS; i > RegStack ; i--) {

	/* if pointer isn't NULL then mark the list, see note above */
	if ( *i != NULL )
		mrklist( *i );
   }

   /* mark the expression stack */
   for (i = Top_Expr; i > ExprStack ; i--)
	mrklist( *i );

   /* mark the value stack */
   for (i = Top_Val; i > ValStack ; i--)
	mrklist( *i );

   /* mark the function stack */
   for (i = Top_Func; i > FuncStack ; i--)
	mrklist( *i );

   /* mark the environment; see notes in eval.c about the environment */
   if ( glo_env )
	mrkatom( glo_env );
}

/* GC() - Garbage Collection.  Gathers all unneeded nodes and puts them on
	the free-list.  returns TRUE if successful, returns FALSE if failed.
*/
static BOOL GC()
{
   SEGMENT *cseg;	/* current segment */
   CONS ccons;		/* current cons node */
   int i;		/* # of cons nodes */
   int used, rec;	/* for stats: used nodes, recovered nodes */

   if ( !torture )
	assert( free_cons == NULL );		/* should we be here? */

   /* we've gotta have mem inorder to GC */
   if ( store == NULL ) return FALSE;

   GC_DEBUG("\nGC: marking, ");

   /* mark all nodes in use */
   mark_all();

   GC_DEBUG("collecting, ");

   used = rec = 0;
   /* step thru mem, collecting cons nodes not marked */
   /* reset mark on used nodes for next GC */
   for ( cseg = store ; cseg != NULL ; cseg = cseg->next) {
	for ( i = 1, ccons = cseg->first ; i <= ALLOC_CONS ; ++i, ++ccons ) {

	   /* skip nodes already on the free-list */
	   if ( mcKind(ccons) != FREE ) {
		if ( mark(ccons) == FALSE ) {	/* not being used? */
			++rec;
			old_cons(ccons);
		} else {
			++used;
			mark(ccons) = FALSE;	/* setup for next GC */
		}
	   }
	}
   }

   if ( gc_debug )
	printf("Used: %d  Recovered: %d Free-list: %d\n", used, rec, cnt_free());

   return (free_cons != NULL);
}

/* ----------------------------------------------------------------------- */
/*                         DEBUGGING ROUTINES                              */
/* ----------------------------------------------------------------------- */

/* count the number of nodes on the cons free list */
static int cnt_free()
{
   int i;
   CONS temp;

   if (free_cons == NULL) return 0;
   for (i = 1, temp = free_cons ; next_free(temp) != NULL ; i++)
	temp = next_free(temp);

   return i;
}
