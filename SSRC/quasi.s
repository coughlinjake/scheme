;;; Quasi-quote expander - Version 1
;;;	written by Jason Coughlin
;;;
;;;	NOTE: Doesn't handle nested quasiquotations.

;; (define (cadr l) (car (cdr l)))

;; (quasiquote exp) => special form that selectively unquotes elements.
(define-form (quasiquote *exp*) (qq *exp* (the-environment)))

;; (qq exp)
;;	atom		=>	atom
;;	,exp		=>	(eval exp)
;;	,@exp		=>	illegal syntax
;;	( ,@exp )	=>	(append (eval (car exp)) (qq (cdr exp)))
;;	( elem elem )	=>	(cons (qq (car exp)) (qq (cdr exp)))
(define (qq exp env)
   (cond
	( (null? exp)			exp )
	( (not (pair? exp))		exp )
	( (eq? (car exp) 'unquote)	(eval (cadr exp) env) )
	( (is-splice? exp)		(error "illegal quasiquote syntax") )
	( (is-splice? (car exp))	(append
						(eval (cadr (car exp)) env)
						(qq (cdr exp) env)) )

	( else				(cons
						(qq (car exp) env)
						(qq (cdr exp) env)) )
   )
)

;; (is-splice? x) - Returns #t if the expression is the (unquote-splice ...)
;;	element.
(define (is-splice? x)
   (and (pair? x) (eq? (car x) 'unquote-splice))
)
