;;; Prop.s -- Property lists
;;;	Version 1 -- Jason Coughlin
;;;
;;;	( symbol . (p1 v1 p2 v2 p3 v3 ...))

;;; (putprop symbol value property)
;;;	Changes the value if the property is already defined.
(define (putprop s v p)
   (let ( (v1		(member p s)) )

	(if (null? v1)
		(set! s (cons (list p v) s))

		(begin
		   (remprop s p)
		   (set! s (cons (list p v) s))
		)
	)
   )
)

;;; (remprop symbol property)
(define (remprop s p)
)

;;; (getprop symbol property)
(define (getprop s p)
   (let ( (v		(member p s)) )

	(cadr v))
)

;;; (proplist symbol)
(define (proplist symbol) symbol)
