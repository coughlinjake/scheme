;; (ifib n) -- Interpreted fibonacci
(define ifib
   (lambda (n)
	(if (< n 2) n (+ (ifib (- n 1)) (ifib (- n 2))))
   )
)

(ifib 4)
(ifib 5)
(ifib 6)
(ifib 7)
(ifib 8)
(ifib 15)

(exit)
