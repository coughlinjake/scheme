;;; boot1.s -- Second bootstrap.  This program compiles and loads the
;;;	primitives and macro system then dumps the environment.
;;;
;;;	1. Define LOAD-AND-COMPILE.
;;;	2. Load and compile the primitives.
;;;	3. Load and compile the macro expander functions.
;;;	4. Define the macros.
;;;	5. Load and compile the system expander.
;;;	6. Quit.
;;;

;; -----------------------------------------------------------------------
;;                           LOAD AND COMPILE
;; -----------------------------------------------------------------------

;; (load-and-compile fname) -- Opens the file named ``fname''.  Compiles and
;;	evaluates each expression in fname.
(define (load-and-compile fname)
   (load-port (open-input-file fname))
)

;; (load-port port) -- Takes an input port, reads each expression from the
;;	port, compiles it, and evaluates it.
(define (load-port port)
   (if (compile-expression (read port))
	(load-port port)
	#t
   )
)

;; (compile-expression e) -- Compiles expression e.  Returns #t if the
;;	expression could be compiled; #f if not.  Returns #f when the
;;	expression is *EOF*.
(define (compile-expression e)
   (if (eof-object? e)

	; can't compile *EOF*
	#f

	; compiled and evaluated this expression => #t
	(begin
		(display "Compiling: ")
		(write e)
		(newline)
		(eval ( *compile* e))
		#t
	)
   )
)

;; -----------------------------------------------------------------------
;;                         FINISH BOOTSTRAPPING
;; -----------------------------------------------------------------------

(display "Compile-loading primitives ...")
(newline)
(load-and-compile "prims.s")
(newline)

(display "Compile-loading macro expander functions ...")
(newline)
(load-and-compile "macfunc.e")
(newline)

(display "Compile-loading system expander ...")
(newline)
(load-and-compile "expand.e")
(newline)

(display "Defining macros ...")
(load "macdef.s")
(newline)

(dump-environment "scheme.img")

(exit)
