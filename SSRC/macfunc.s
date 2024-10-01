;;; MACFUNC.S -- Expander functiosn for for let, named-let, let*, unless,
;;;	when, cond, letrec
;;;
;;; * Notice that I don't use quasiquote.  I would like to implement
;;;	quasiquote as a macro instead of a special form.
;;;
;;; * The function ``map1'' must already be defined.
;;;
;;; * The expander functions are defined as separate functions so they
;;;	will be compiled.  If the expander functions were anonymous
;;;	lambda's they wouldn't be compiled.  Eg, the following causes the
;;;	expander function for ``unless'' to be interpreted rather than
;;;	compiled.
;;;
;;;	(macro unless
;;;	   (lambda (e) ...))
;;;

;; -----------------------------------------------------------------------
;; COND
;; -----------------------------------------------------------------------

;; (cond (test e1 e2 ...) more ...)
(define expand-cond
   (lambda (e)
      ; (cond) => #f
      (if (null? (cdr e)) #f

	; (cond an-atom ... ) => error
	(if (not (pair? (cadr e))) (error "illegal cond syntax")

	   ; (cond (else e1 e2 ...)) => (begin e1 e2 ...)
	   (if (eq? 'else (caadr e)) (append '(begin) (cdadr e))

		; (cond (test) more ...) => (or test (cond more ...))
		(if (null? (cdadr e)) (list 'or (caadr e) (append '(cond) (cddr e)))

		   ; (cond (test e1 e2 ...) more ...) =>
		   ;    (if test (begin e1 e2 ...) (cond more ...))
		   (list 'if (caadr e) (append '(begin) (cdadr e))
			(append '(cond) (cddr e)))
		)
	   )
	)
      )
   )
)

;; -----------------------------------------------------------------------
;; LET, LET*, named LET
;; -----------------------------------------------------------------------

;; expand let forms.
(define expand-let
   (lambda (e)
	(cond
	   [ (null? (cadr e))	(append '(begin) (cddr e)) ]
	   [ (atom? (cadr e))	(expand-named-let e) ]
	   [ else		(expand-let2 e) ]
	)
   )
)

;; (expand-let2 e)
;;	(let ( [id val] ... ) body ) == ( (lambda [id ...] body) val ... )
(define expand-let2
   (lambda (e)
	(cons
	   (append
		; (map car (cadr e)) extracts the identifiers
		[list 'lambda (map1 car (cadr e))]
		[cddr e]
	   )

	   ; extract vals
	   (map1 cadr (cadr e))
	)
   )
)

;; (expand-named-let e)
;;	(let name ( [id val] ... ) body ) ==
;;		(letrec ( [name   (lambda (id ...) body)] ) (name val ...))
(define expand-named-let
   (lambda (e)
	(list
	   'letrec

	   (list
		(list [cadr e]
		   [append
			; append: (lambda ( ids ...)) && body
			(if (null? (caddr e)) '() (list 'lambda (map1 car (caddr e))))
			(cdddr e)
		   ]
		)
	   )

	   (cons
		(cadr e)
		(map1 cadr (caddr e))
	   )
	)
   )
)

;; (let* ( [id1 val1] [id2 val2] ...) body ) ==
;;	(let ( [id1 val1] )
;;	   (let* ( [id2 val2] ... ) body ))
(define expand-let*

   (lambda (e)
	(if (null? (cadr e))
	   (append '(begin) (cddr e))
	   (list
		'let
		(cons (caadr e) '())
		(append
		   (list
			'let*
			(cdadr e) )

		   (cddr e)
		)
	   )
	)
   )
)

;; -----------------------------------------------------------------------
;; LETREC
;; -----------------------------------------------------------------------

(define letrec-bind
   (lambda (e)
      (list (car e) #t)
   )
)

(define letrec-set
   (lambda (e)
      (list 'set! (car e) (cadr e))
   )
)

(define expand-letrec
   (lambda (e)
      (append
	 '(let)
	 (list (map1 letrec-bind (cadr e)))
	 (map1 letrec-set (cadr e))
	 (cddr e)
      )
   )
)

;; -----------------------------------------------------------------------
;; UNLESS
;; -----------------------------------------------------------------------

;; (unless test e1 e2 ...) == (if (not test) (begin e1 e2 ...))
(define expand-unless
   (lambda (e)
	(list
	   'if
	   [list 'not (cadr e)]
	   [append '(begin) (cddr e)]
	)
   )
)

;; -----------------------------------------------------------------------
;; WHEN
;; -----------------------------------------------------------------------

;; (when test e1 e2 ...) == (if test (begin e1 e2 ...))
(define expand-when
   (lambda (e)
	(list
	   'if
	   (cadr e)
	   (append '(begin) (cddr e))
	)
   )
)
