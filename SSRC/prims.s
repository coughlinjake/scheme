;;; The member function

(define memberDb '( ( (member (? x) (@ (? x) ! (? t))) )
              ( (member (? x) (@ (? h) ! (? t))) (member (? x) (? t)) )
            )
)

;;; The append function

(define app2 '( (append (@ ) (? b) (? b)) ) )
(define app3 '( (append (@ (? h) ! (? t)) (? b) (@ (? h) ! (? c))) 
     (append (? t) (? b) (? c)) ) )

(define appendDb (list app2 app3))

;;; The Reverse list function

(define rev1 '( (rev (@ ) (@ )) ) )
(define rev2 '( (rev (@ (? h) ! (? t)) (? l)) (rev (? t) (? z)) 
					      (append (? z) (@ (? h)) (? l))))
(define revdb (list rev1 rev2))

;;; The efface function which deletes the first occurance of an element.

(define eff1 '( (efface (? a) (@ (? a) ! (? l)) (? l)) ) )
(define eff2 '( (efface (? a) (@ (? b) ! (? l)) (@ (? b) (? m)))
		      (efface (? a) (? l) (? m))) )
(define effacedb (list eff1 eff2))

;; The delete function which deletes ALL occurrances of an element.

(define del1 '( (delete (? ggg) (@) (@))) )

