/* Global definitions for Scheme. */

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>

/* useful constants */
#define TRUE		1
#define FALSE		0
#define EOS		'\000'
#define BOOL		int

/* cell types for a CONS node */
#define NULLNODE	0		/* same as NULL */
#define NILNODE		1		/* Scheme '() */
#define PAIR		2
#define INT		3
#define FLOAT		4
#define SYMBOL		5
#define CHAR		6
#define STRING		7
#define PORT		8
#define BCODES		9
#define EXEPOINT	10
#define CFUNC		11		/* primitive function */
#define CFORM		12		/* primitive form */
#define CLOSURE		13		/* user-defined function */
#define FORM		14		/* user-defined form */
#define CONT		15
#define RESUME		16
#define VECTOR		17
#define ENVMNT		18
#define TOBJ		19		/* Scheme TRUE */
#define FOBJ		20		/* Scheme FALSE */
#define EOFOBJ		21		/* Scheme EOFOBJ */

/* cell_type for MM */
#define FREE		50

/* port types */
#define INPUT		1
#define OUTPUT		2
#define CLOSED		3

struct Pair {
	struct C *car;
	struct C *cdr;
} ;

struct Port {
	FILE *fp;
	int type;		/* INPUT, OUTPUT, or CLOSED */
} ;

struct Vector {
	unsigned int size;		/* # of elements */
	struct C **elems;		/* the elements */
} ;

struct B_Code {
	unsigned char *code;		/* compiled code */
	struct C **const_table;		/* constant table */
	unsigned int lcode;		/* last byte of code */
	unsigned int lconst;		/* last constant */
} ;

struct E_Pnt {
	struct C *bcode;	/* pntr to compiled code */
	struct C *env;		/* current environment */
	int pc;			/* offset w/in compiled code */
} ;

struct C_Fnc {
	char *name;		/* name of function */
	int rargs;		/* # of required args */
	int aargs;		/* # of allowed args */
	int prnum;		/* predefined # */
	void (*op)( C_VOID );	/* interpreted operation */
} ;

/* defn of user-defined closures AND user-defined special forms */
struct Closure {
	struct C *env;		/* saved env */
	struct C *parms;	/* paramters */
	struct C *body;		/* body of procedure */
} ;

/* defn of a continuation */
struct Continuation {
	struct C *env;		/* env in effect at capture */
	struct C *vals;		/* value stack at capture */
	struct C *fncs;		/* function stack at capture */
	struct C *exps;		/* expression stack at capture */
} ;

/* defn of an environment */
struct Envmnt {
   struct C *nested;		/* nested bindings (an A-LIST) */
   struct C *global;		/* global bindings (a VECTOR) */
} ;

/* the cons node datatype */
struct C {
   int cell_type;		/* PAIR, INT, FLOAT, SYMBOL, CHAR, etc. */
   BOOL mmark;

   union {
	struct Pair cons_node;
	struct B_Code bcode;
	struct E_Pnt  exepnt;
	struct Port   port;
	struct C_Fnc func;
	struct Closure closure;
	struct Continuation cont;
	struct Vector vector;
	struct Envmnt env;

	int int_data;		/* also is symbol table entry index */
	REAL_NUM float_data;
	char *string;
	char character;
   } data;
} ;
typedef struct C CONSNODE;
typedef struct C *CONS;

/* global variables */
extern CONS AStack[];
extern CONS *Top;
extern FILE *currin, *currout;

/* special system CONS nodes */
extern CONS NIL;			/* empty list */
extern CONS T;				/* Scheme: Boolean true value */
extern CONS F;				/* Scheme: Boolean false value */
extern CONS EOF_OBJ;			/* Scheme: EOF object */
extern CONS STDIN;			/* current input port */
extern CONS STDOUT;			/* current output port */
extern CONS EXP_TABLE;			/* symbol: *EXPANSION-TABLE* */
extern CONS CALL;
extern CONS MARK;
extern CONS PUSHFUNC;
extern CONS RESTORE;
extern CONS EXP_RESUME;

extern CONS glo_env;			/* see notes in eval.c */

extern jmp_buf tlevel;

/* global macros */
#define StrSame(s, p)	( strcmp( (s), (p) ) == 0 )

/* proto-types */
void InitGlos( C_VOID );
void CatchSig( C_INT );
