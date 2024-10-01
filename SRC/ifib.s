;; (ifib n) -- Interpreted fibonacci
(define ifib
   (lambda (n)
	(if (< n 2)
	   n
	   (+ (ifib (- n 1)) (ifib (- n 2)))
	)
   )
)

(ifib 3)
(ifib 5)
(ifib 7)
(ifib 9)
(ifib 15)

(exit)
