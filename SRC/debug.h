/* debug.h - Contains the debugging information! */

/* global debugging information variables */
extern int gc_debug;
extern int sym_debug;
extern int eval_debug;
extern int cp_debug;
extern int torture;

#define GC_DEBUG(e)	if ( gc_debug ) fprintf( currout, "%s", e)
#define CP_DEBUG(x,g)	if ( cp_debug ) { fprintf( currout, "%s", x); mcWrite(g, currout); }
#define EV_DEBUG(e,l)	if ( eval_debug ) { fprintf( currout, "%s", e); mcWrite(l, currout); }
#define SYM_DEBUG(e)	if ( sym_debug ) fprintf( currout, "%s", e)
