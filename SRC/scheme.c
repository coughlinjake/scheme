/* scheme.c - A REP loop which reads a list, evaluates it, and prints it out.

	All Scheme code in C copyright (C) 1990 by Jason Coughlin.  You are
	granted permission to make unlimited copies of this source code
	and to distribute these copies provided:

		(1) You distribute the source code in all copies.
		(2) You distribute the copyright message in all copies.
		(3) You charge nothing more than the cost of the disk or
		    tape on to which you copy it, mailing charge, or some
		    other similar nominal fee.

	Please feel free to make modifications.  However, I would appreciate
	hearing from you so that your changes can benefit everyone using
	this program.

	I am in debt to Dr. Gary Levin for the ideas, the motivation, the
	bug fixes, and the guidance.  Without his help, this program would
	have never seen a CPU.

	Extend-syntax idea courtesy of Eugene Kohlbecker.  Extend-syntax
	sources courtesy of R. Kent Dybvig.
*/

#include "machine.h"

#include <signal.h>

#include "glo.h"
#include "scanner.h"
#include "memory.h"
#include "micro.h"
#include "symstr.h"
#include "eval.h"
#include "compile.h"
#include "error.h"

#define VERSION		"1.2a"		/* this version # */

/* globals */
jmp_buf tlevel;				/* top level for long jump */
FILE *currin, *currout;			/* where I/O comes/goes */

/* local prototypes */
void usage( C_VOID );
void load_init( C_VOID );

main(argc, argv)
int argc;
char *argv[];
{
   CONS lyst;
   int l, silent;

   silent = FALSE;
   currin = stdin;
   currout = stdout;

   /* command line arguments */
   for ( l = 0; l < argc; l++ ) {
	if ( argv[l][0] == '-' || argv[l][0] == '/' ) {
		switch ( argv[l][1] ) {
		   case 's':
			silent = TRUE;
			break;

		   case '?':
			usage();
			exit(0);
		}
	}
   }

   if ( !silent ) {
	fprintf(stdout, "\nScheme - Version %s\n", VERSION);
	fprintf(stdout, "Copyright (C) 1990 by Jason Coughlin\n\n");
   }

   /* initialize the different pieces */
   InitScanner();
   InitSymstr();
   InitMem(argc, argv);
   InitMicro();
   InitEval(argc, argv);
   InitComp(argc, argv);
   InitGlos();		/* initialize after lower levels */

   /* levels initialized, get memory */
   GetMem();

   /* load the initialization file */
   load_init();

   /* initialization file loaded; on an error, we come here */
   setjmp(tlevel);

   /* whenever we return to the top-level, we need a clean slate so
    * reset system variables and stacks.
    */
   signal(SIGINT, CatchSig);
   mcClearStacks();
   currin = stdin;
   currout = stdout;

   /* read-eval-print loop */
   for ( ; ; ) {
	/* read */
	fprintf( currout, "[=> ");
	lyst = mcRead( currin );

	/* eval */
	mcPushExpr(lyst);
	evEval();

	/* print */
	fprintf( currout, "\n" );
	lyst = mcPopVal();
	mcWrite( lyst, currout );
	fprintf( currout, "\n");
   }
}

/* CatchSig() - Catch CTRL-C from user and return to Top level. */
void CatchSig(sig)
int sig;
{
#ifdef NO_TRAP
   fprintf( currout, "\n*Return-to-Top-Level*\n");
   ERROR;
#else
   exit(0);
#endif
}

/* usage() - Print the command line options. */
void usage()
{
   printf("\nScheme - Version %s\n", VERSION);
   printf("Copyright (C) 1989 by Jason Coughlin\n\n");
   printf("Command line options:\n");
   printf("\t-?\t\tDisplay command-line options and quit.\n");
   printf("\t-c\t\tCompiler debug ON - Dump compiler statistics.\n");
   printf("\t-e\t\tEval debug ON - Dump evaluation statistics.\n");
   printf("\t-g\t\tGC debug ON - Dump garbage-collection stats.\n");
   printf("\t-t\t\tTorture test ON - GC before every allocation.\n");
   printf("\t-s\t\tSilent Mode - Skip startup header.\n");
   printf("\n");
}

/* load_init() - Load SCHEME.INI */
void load_init()
{
   static loaded = FALSE;

   /* if there is an error in scheme.ini then where to jump to for
    * the top-level and where to receive and send I/O must be
    * defined.
    */
   setjmp(tlevel);
   currout = stdout;
   currin = stdin;

   /* there are two reasons that we could be here:
    *	(a) error in scheme.ini caused a longjmp() here
    *	(b) load_init() was invoked.
    */
   if ( loaded ) {

	/* case (a) -- an error in scheme.ini */
	fprintf(stderr, "Error in SCHEME.INI -- can't recover.\n");
	exit(1);

   } else {

	/* case (b) -- main() invoked load_init()
	 *
	 * set loaded because we won't return from evEval()
	 * until the ENTIRE scheme.ini file has been loaded.
	 * if loaded wasn't set then an error would cause an
	 * infinite loop.
	 */
	loaded = TRUE;

	if ( mcLoad("scheme.ini") )
		evEval();
   }
}
