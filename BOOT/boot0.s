;;; boot0.s -- First bootstrap.  This program performs the first boot-strap
;;;	of getting the macro and expansion system ready for the primitive
;;;	compiler.
;;;
;;;	1. Load the primitives.
;;;	2. Load the macro expander functions.
;;;	3. Define the macros.
;;;	4. Load the expander.
;;;	5. Expand the macro expander functions so the primitive compiler
;;;	   can compile them.
;;;	6. Quit.
;;;

;; -----------------------------------------------------------------------
;;                              EXPAND FILE
;; -----------------------------------------------------------------------

;; (expand-file iname oname)
(define (expand-file iname oname)
   (expand-port (open-input-file iname) (open-output-file oname))
)

;; (expand-port iport oport)
(define (expand-port iport oport)
   (let ([exp		(read iport)])
	(if (eof-object? exp)
	   #t
	   (begin
		(display "Expanding: ")
		(write exp)
		(newline)

		(write (expand exp) oport)
		(newline oport)
		(expand-port iport oport)
	   )
	)
   )
)

;; -----------------------------------------------------------------------
;;                             BOOT-STRAP #0
;; -----------------------------------------------------------------------

(display "Loading primitives ...")
(load "prims.s")
(newline)

(display "Loading macro expander functions ...")
(load "macfunc.s")
(newline)

(display "Defining macros ...")
(load "macdef.s")
(newline)

(display "Loading expander ...")
(load "expand.s")
(newline)

(display "Expanding the macro expander functions ...")
(newline)
(expand-file "macfunc.s" "macfunc.e")
(newline)

(display "Expanding the system expander ...")
(newline)
(expand-file "expand.s" "expand.e")
(newline)

(display "Done.  Now restart Scheme and load BOOT1.")
(newline)

(exit)
