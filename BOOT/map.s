
;;; (MAP procedure list1 list2 ...) - Written by R. Kent Dybvig from his
;;;	book _The SCHEME Programming Language_
(define map
   (lambda (f ls . more)
      (if (null? more)

	; just call map1 if this is a 1 arg function
	(let map1 ([ls ls])
	   (if (null? ls)
		'()
		(cons (f (car ls))
		   (map1 (cdr ls)))))

	(let map-more ([ls ls] [more more])
	   (if (or (null? ls) (ormap null? more))
		'()
		(cons (apply f
			(car ls)
			(map car more))

		      (map-more (cdr ls)
				(map cdr more)))))))
)

