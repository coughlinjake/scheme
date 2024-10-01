;;; System expander - Version 1
;;;	written by Jason Coughlin

;; (expand-once exp)
;;	'()			=>	'()
;;	atom			=>	atom
;;	(x a1 a2...) && macro x	=>	(x's expander exp)
;;	(x a1 a2...) && no macro=>	expand on x's components
(define expand-once
   (lambda (exp)
	(if (not (pair? exp))

	   exp

	   (let* ( (exp-bind (assoc (car exp) *EXPANSION-TABLE*))
		   (expander (and (not (null? exp-bind)) (cdr exp-bind))) )

		 (if (null? expander)
			(map1 expand-once exp)
			(expander exp)
		 )
	   )
        )
   )
)

;; (expand exp) -- Fully expand exp.
(define expand
   (lambda (exp)
	(if (not (pair? exp))

	   exp

	   (let ( [exp-bind (assoc (car exp) *EXPANSION-TABLE*)] )

		(if (null? exp-bind)
			(map1 expand exp)
			(expand ( (cdr exp-bind) exp))
		)
	   )
	)
   )
)

