/* machine.h -- Machine and OS dependencies go here */

/* Uncomment the next line if you're using a traditional K & R
	compiler.
#define TRAD
*/

/* AllocH - Define this if your compiler requires you to include alloc.h.
	Note that TCC *requires* this -- otherwise you'll get unpredictable
	results.  Comment the line if your compiler doesn't require (or
	define) this header file.
#define AllocH
*/

/* STRING_H - Define this to the name of the appropriate header file. */
#define STRING_H	<string.h>

/* MEMORY_H - Define this to the name of the appropriate header file. */
#define MEMORY_H	<mem.h>

/* if you don't have memcpy(), define NoMemcpy.  if you have bcopy()
	and you don't have memcpy(), define Bcopy and NoMemcpy.
#define Bcopy
#define NoMemcpy
*/
#define Memcpy

/* Define one of these based on your compiler.
#define REAL_NUM	float
#define REAL_FORMAT	"%f"
*/
#define REAL_NUM	double
#define REAL_FORMAT	"%lf"

/* this is the string used to open a file for reading binary info.  TCC and
	MSC require this to be "rb" for binary.
#define FILE_READ_BIN	"r"
*/
#define FILE_READ_BIN	"rb"

/* this is the string used to open a file for writing binary info.  TCC and
	MSC require this to be "wb" for binary.
#define FILE_WRITE_BIN	"w"
*/
#define FILE_WRITE_BIN	"wb"

/* ----------------------------------------------------------------------- */
/*                End of user configurable parameters.			   */
/* ----------------------------------------------------------------------- */

/* STDLIB_H */
#ifdef TRAD
#	define STDLIB_H		"stdlib.h"
#else
#	define STDLIB_H		<stdlib.h>
#endif

/* definitions for traditional compilers */
#ifdef TRAD
#	define C_VOID
#	define C_INT
#	define C_CHAR
#	define C_FILE
#	define C_CONS
#	define C_PTR
#	define C_VOID_F_PTR
#	define C_CONS_F_PTR
#	define C_CODE_BUFFER
#	define C_ARRAY
#	define X
#else		/* defs for ANSI compilers */
#	define C_VOID	void
#	define C_INT	int
#	define C_CHAR	char
#	define C_FILE	FILE
#	define C_CONS	CONS
#	define C_PTR	*
#	define C_VOID_F_PTR	void (*)( void )
#	define C_CONS_F_PTR	void (*)( CONS )
#	define C_CODE_BUFFER	CODE_BUFFER
#	define C_ARRAY	[]
#	define X	,
#endif
