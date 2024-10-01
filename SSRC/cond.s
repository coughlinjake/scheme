(define cond-exp
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
