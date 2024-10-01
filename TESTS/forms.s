(if (null? ()) 0 1)
(if (null? T) 0 1)
(if (null? '(this is a test)) #null #t)
(if (null? #null) #null #t)
(if (null? #f) #null #t)
(quote (this is a test))
'(this is a test)
(define a '(a b c))
(define b a)
a
b
(set-car! b '(1 2))
(set-cdr! a '(3))
b
a
(define (length lyst) (if (NuLL? lyst) 0 (+ 1 (length (cdr lyst)))))
(length a)
(length (cons a b))
(and (= 2 2) (> 2 1))
(and (= 2 2) (< 2 1))
(and 1 2 'c '(f g))
(and)
(or (= 2 2) (> 2 1))
(or (< 2 1) (= 2 2))
(or #null #null #null)
(or (assoc 'b '( (a . 1) (c . 2) (b . 3))) (/ 3 0))
(or)
(cond
	( (null? T)		'NOPE )
	( (or)			'SORRY )
	( else			'GREAT )
)

(cond
	( (null? T)		'NOPE )
	( (or T)		'GREAT )
	( T			'SORRY )
)

(cond
	( (null? 'hello)	'NOPE )
	( (or)			'SORRY )
	( else			'GREAT )
)

(cond
	( (> 3 2)		'greater )
	( (< 3 2)		'less )
)

(cond
	( (> 3 3)		'greater )
	( (< 3 3)		'less )
	( (= 3 3)		'equal )
)

(exit)

