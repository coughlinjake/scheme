/* error.h - Contains the error code. */

#include "debug.h"

void evError( C_VOID );

/* global error macros */
#define ERROR		evError()
#define MSGLN(t,x)	fprintf( currout, "\n%s: %s\n", t, x)
#define MSG(t,x)	fprintf( currout, "\n%s: %s", t, x)
#define RT_ERROR(s)	{MSGLN("Error", s); ERROR;}
#define RT_LERROR(s,l)	{MSG("Error", s); mcWrite(l, currout); fprintf( currout, "\n"); ERROR;}
#define WARNING(s)	MSGLN("Warning",s)
#define FATAL(s)	{MSGLN("FATAL", s); exit(1);}
