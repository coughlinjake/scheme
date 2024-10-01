;; (cfib n) -- Compiled fibonacci
(eval (compile '
   (define cfib
	(lambda (n) (if (< n 2) n (+ (cfib (- n 1)) (cfib (- n 2))))))))

(cfib 3)
(cfib 5)
(cfib 7)
(cfib 9)
(cfib 15)

(exit)
