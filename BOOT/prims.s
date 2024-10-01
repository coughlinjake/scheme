;;; prims.s - Primitive Scheme procedures Ver 1.0
;;;	Written by Jason Coughlin
;;;
;;;	These are the lowest level.  These are primitive Scheme operations
;;; that don't require a macro definition.
;;;
;;;	NOTE: Since there are no macro or compiler syntax definitions,
;;; you can't use the special form of define: (define (func arg1 ...) body)
;;; This must be explicitly written: (define func (lambda (arg1 ...) body))

;; -----------------------------------------------------------------------
;; The 24 cXr procedures.
;; -----------------------------------------------------------------------
(define cadr
   (lambda (l) (car (cdr l)))
)
(define caddr
   (lambda (l) (car (cdr (cdr l))))
)
(define cddr
   (lambda (l) (cdr (cdr l)))
)
(define cdddr
   (lambda (l) (cdr (cdr (cdr l))))
)
(define caadr
   (lambda (l) (car (car (cdr l))))
)
(define cdadr
   (lambda (l) (cdr (car (cdr l))))
)

;; -----------------------------------------------------------------------
;; MAP1
;; -----------------------------------------------------------------------

;;; map1 -- Code taken from _The Scheme Programming Language_ by
;;;	R. Kent Dybvig.
(define map1
   (lambda (f ls)
      (if (null? ls)
	'()
	(cons (f (car ls))
	      (map1 f (cdr ls)))
      )
   )
)

;; -----------------------------------------------------------------------
;; BOXES
;; -----------------------------------------------------------------------

(define box
   (lambda (l)
	(cons '*BOX* l)
   )
)

(define boxed-obj?
   (lambda (b)
	(eq? (car b) '*BOX*)
   )
)

(define unbox
   (lambda (b)
	(if (boxed-obj? b) (cdr b) (error b "is not a box"))
   )
)

(define set-box!
   (lambda (b l)
	(if (boxed-obj? b) (set-cdr! b l) (error b "is not a box"))
   )
)
