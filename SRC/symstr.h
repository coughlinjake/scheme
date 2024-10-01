/* symstr.h */

#define MAX_SYMBOLS	1000	/* # of different symbols */

extern char *SymTable[];
extern char *STRING_SPACE;
extern int Sleft;

/* prototypes */
void InitSymstr( C_VOID );
int ssAddSymbol( C_CHAR C_PTR );
int ssIsSymbol( C_CHAR C_PTR );
char *ssAddString( C_CHAR C_PTR );
char *ssSubstring( C_CHAR C_PTR X C_INT X C_INT );
char *ssAppend( C_CHAR C_PTR X C_CHAR C_PTR );
