
;; -----------------------------------------------------------------------
;; DEFINE
;; -----------------------------------------------------------------------

;; expand define form
(define expand-define
   (lambda (e)
	(if (not (pair? (cadr e)))
	    e
	   (list
		'define
		(caadr e)
		(append
		   '(lambda)
		   (list (cdadr e))
		   (cddr e)
		)
	   )
	)
   )
)
