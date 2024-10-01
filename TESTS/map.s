;; Procedure MAP - (MAP PROC ARGS) Apply PROC to each element of list
;;	ARGS, returning a list of the applications.
;; (MAP car '( (this) (is) (a) (test) ) ) => (this is a test)
(define (map proc args)
   (if (null? args) ()
	(cons
	   (apply proc (cons (car args) ()))
	   (map proc (cdr args))
	)
   )
)

;; Procedure FOR-EACH - Same as MAP except that FOR-EACH guarantees that
;;	the elements of ARGS are evaluated sequentually.  They are in my
;;	definition so no need to re-write it.  (I LOVE Scheme :-).
(define for-each map)
