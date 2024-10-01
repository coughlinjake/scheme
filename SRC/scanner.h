/* scanner.h */

/* token types */

/* already defined in glo.h:
	#define INT
	#define FLOAT
	#define SYMBOL
	#define CHAR
	#define STRING
	#define PORT
*/
#define NO_TOKEN	20	/* no token */
#define LP		21	/* left paren */
#define RP		22	/* right paren */
#define LB		23	/* left bracket [ */
#define RB		24	/* right bracket ] */
#define DOT		25	/* the dot: . */
#define QUOTE		26	/* the ' */
#define QUASI		27	/* the ` */
#define UNQUOTE		28	/* the , */
#define UNQ_SPLICE	29	/* the ,@ */
#define UNKNOWN		100	/* unknown token! */

#define CDELIM		';'	/* comment delimiter */

#define UNKNOWN_ELEMS	-1	/* unknown # of vector elements */

/* variables */
extern char token[];			/* the current token's string */
extern int token_type;			/* the current token type */
extern int inum;			/* token: integer value */
extern REAL_NUM fnum;			/* token: float value */

/* proto-types */
void InitScanner( C_VOID );
int GetToken( C_FILE C_PTR );
void PutBack( C_VOID );
