
(define (while a b)
   (if (a)
	(begin b (while a b))
	1
   )
)

(define (c) (if (= i 0) #!false #!true) )

(define (f) (set! i (- i 1)))

(define i 1000)
(gcdebug)
(while c f)
i
(exit)