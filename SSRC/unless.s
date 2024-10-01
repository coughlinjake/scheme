;;; (UNLESS TEST E1 E2 ...)
;;;	if TEST then ok
;;;	else begin E1 E2 ...
(define-form (unless *unless-test* . *unless-exps*)
   (if (eval *unless-test*)
	#t
	(eval (append '(begin) *unless-exps*))
   )
)
