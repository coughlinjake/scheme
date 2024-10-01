(procedure? car)
(procedure? 'car)
(procedure? (lambda (x) (* x x)))
(call/cc procedure?)
(apply + (list 3 4))

(define (add1 h) (+ h 1))

(define compose
   (lambda (f g)
     (lambda args
       (f (apply g args)))))

((compose add1 *) 5 4)

(exit)
