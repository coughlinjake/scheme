;;; Boxes.s -- Version 1
;;;	written by Jason Coughlin

;; (Box obj) - Boxes obj and returns the result.
(define (box l) (cons '*BOX* l))

;; (Boxed-obj? obj) - Returns #t if obj is a box.
(define (boxed-obj? b) (eq? (car b) '*BOX*))

;; (Unbox b) - Unboxes box b.
(define (unbox b)
   (if (boxed-obj? b) (cdr b) (error b "is not a box"))
)

;; (Set-Box! b obj) - Changes the contents of box b to be obj.
(define (set-box! b l)
   (if (boxed-obj? b) (set-cdr! b l) (error b "is not a box"))
)
