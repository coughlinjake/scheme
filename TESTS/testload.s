(define list-length
   (lambda (obj)
	(call/cc
	   (lambda (return)
		(letrec ((r
			   (lambda (obj)
				(cond	( (null? obj) 0 )
					( (pair? obj) (+ (r (cdr obj)) 1))
					( else (return #f))))))
			(r obj))))))
