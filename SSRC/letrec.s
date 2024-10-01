(define test-exp1
   '(letrec ((even?
		(lambda (x)
		   (or  (zero? x)
			(odd? (- x 1)))))

	     (odd?
		(lambda (x)
		   (and (not (zero? x))
			(even? (- x 1)))))
	    )
	(even? 20)
    )
)

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
	 (list (map letrec-bind (cadr e)))
	 (map letrec-set (cadr e))
	 (cddr e)
      )
   )
)
