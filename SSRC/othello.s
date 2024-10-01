;;; Othello by Ted McManus                                  (c) Spring 1990
;;;
;;;   This program plays a game of Othello against a human opponent.  The
;;;  program uses a modified minimax search strategy to a depth of
;;;  *D-BOUND* ply to determine its move.  The static evaluation payoff
;;;  function is pretty basic: Count the # of squares held by each player,
;;;  with each square worth 1 point (opponent's squares are negative).
;;;  Corners are worth 50.  If a corner is empty, then each of the three
;;;  squares adjacent to it count as -10 (since they should be avoided).
;;;  That's it.  When the search function is given a board, it generates
;;;  all possible children of that board, runs the payoff function on each
;;;  one, assumes that the other player will not make a stupid move, and so
;;;  it then discards all but the best *N* children.
;;;   With low values for *D-BOUND* and *N*, this approximates a hill-climbing
;;;  strategy, as the payoff function concentrates mostly on the number of
;;;  pieces each side has, rather than strategic position (with the exception
;;;  of corners).  An alpha-beta search would be useful here, especially
;;;  since the children are already pre-sorted in best-to-worst order, which
;;;  tends to maximize pruning.  As things are, with the default settings of
;;;  *N* = 3 and *D-BOUND* = 3, a move is generated in an almost constant
;;;  time of 30 seconds.
;;;   To begin a game, call function (GO).
;;;   To enter a pass, enter (8 8).


(define GloBoard)
(define backup)
(define *N* 3)
(define *D-BOUND* 3)
(define symbols (vector "@" " " "O"))
(define *MOVES*)
(define *PASS*)
(define *GO-FIRST*)
(define me 1)
(define opp -1)
(define white)
(define (pay-fn b) (payoff3 b))
(define (gen-fn b who) (gen-kids b who))
(define (p w) (print-board w))
(define (test-fn) (gen-kids globoard me))

(define (GO)
  (init)
  (PLAY)
  (game-done))

(define (init)
 (set! white (choose-sides))
 (set! *GO-FIRST* (who-goes-first))
 (set! GloBoard (init-board))
 (set! *MOVES* 0)
 (set! *PASS* 0)
 (set! backup (v-copy globoard)))

(define (choose-sides)
 (newline)
 (display "(W)hite or (B)lack?    -->")
 (let ((c (read-char)))
  (case (or (eq? c #\b) (eq? c #\B))
   (#T (writeln "Black") -1)
   (#F (writeln "White")  1))))

(define (who-goes-first)
 (newline)
 (display "Who goes first?  (W)hite or (B)lack?    -->")
 (let ((c (read-char)))
  (case (or (eq? c #\b) (eq? c #\B))
   (#T (writeln "Black") (minus white))
   (#F (writeln "White") white))))

(define (init-board)
 (let ((board (make-vector 8 3280)))
  (set-square board white 3 4)
  (set-square board white 4 3)
  (set-square board (minus white) 4 4)
  (set-square board (minus white) 3 3)))

(define (PLAY)
 (do ((who (* *GO-FIRST* me) (minus who)))
  ((gameover?))
  (writeln)
  (print-board globoard)
  (set! backup (v-copy globoard))
   (do-move who)))

(define (gameover?)
  (or (= *PASS* 2)
      (= *MOVES* 60)))

(define (do-move side)
  (cond ((= side me) (display "Enter move in format (row col): ")
                     (gc #T)
                     (let* ((mv (read)) (mr (car mv)) (mc (cadr mv)))
                       (if (= 8 mr)
                            (set! *PASS* (1+ *PASS*))
                            (begin
                             (set! globoard (new-board globoard side mr mc))
                             (set! *PASS* 0)
                             (set! *MOVES* (1+ *MOVES*))))))
        ((= side opp) (display "Here's my move....")
                      (let* ((mv (move opp))
                             (mr (vector-ref mv 1))
                             (mc (vector-ref mv 2)))
                       (writeln "(" mr " " mc ")")
                       (if (= 8 mr)
                            (set! *PASS* (1+ *PASS*))
                            (begin
                             (set! globoard (vector-ref mv 0))
                             (set! *PASS* 0)
                             (set! *MOVES* (1+ *MOVES*))))))))


(define (move who)
  (search (vector globoard 0 0 0) (minus who) 0))

(define (search board who lvl)
  (cond ((= lvl *D-BOUND*) board)
        (#T (let* ((c (gen-fn (vector-ref board 0) (minus who)))
                   (c1 (car c))
                   (bb c1)
                   (val (if (= who me) -2000 2000)))
            (do ((sb 0))
             ((null? c) bb)
              (set! sb (search c1 (minus who) (1+ lvl)))
              (cond ((= who me) (cond ((> val (vector-ref sb 3))
                                         (set! bb c1)
                                         (set! val (vector-ref sb 3)))))
                    (#T (cond ((< val (vector-ref sb 3))
                                (set! bb c1)
                                (set! val (vector-ref sb 3))))))
              (set! c (cdr c))
              (set! c1 (car c)))))))

(define (onboard row col)
 (and (> row -1) (> col -1) (< row 8) (< col 8)))

(define (square board row col)
 (-1+ (modulo (quotient (vector-ref board row) (expt 3 col)) 3)))

(define (set-square board color row col)
 (let ( (r1 (vector-ref board row))
        (x (- color (square board row col))) )
  (vector-set! board row (+ r1 (* x (expt 3 col))))))

(define (new-board b color r c)
  (cond ((not (= 0 (square b r c))) '())
	(#T (let ((tot 0) (b1 (v-copy b)))
	     (set-square b1 color r c)
             (do ((x -1 (1+ x)))
               ((= x 2))
               (do ((y -1 (1+ y)))
                 ((= y 2))
                 (let ((x1 0) (y1 0) (n 0))
                  (do ((r1 (+ r x x1) (+ r x x1))
                       (c1 (+ c y y1) (+ c y y1)))
                    ((not (and (onboard r1 c1)
                               (= (-1+ (modulo (quotient (vector-ref b1 r1)
                                                         (expt 3 c1)) 3))
                                  (minus color)))))
                    (set! x1 (+ x1 x))
                    (set! y1 (+ y1 y))
                    (set! n (1+ n)))
                  (when (and ( > n 0)
                             (onboard (+ r x x1) (+ c y y1))
                             (= color (-1+ (modulo (quotient
                                                    (vector-ref b1 (+ r x x1))
                                                    (expt 3 (+ c y y1))) 3))))
                    (do ((z 1 (1+ z)))
                        (( > z n))
                         (set-square b1 color (+ r (* z x)) (+ c (* z y))))
                    (set! tot (+ tot n))))))
             (if (= tot 0) '()
                            b1)  ))))

(define (gen-kids b color)
  (letrec ((foo (lambda (r c b-list)
              (cond ((= r -1) b-list)
                    (#T (let ((b1 (new-board b color r c))
                              (c1 (if (= c 0) 7 (-1+ c)))
                              (r1 (if (= c 0) (-1+ r) r)))
                         (if (null? b1) (foo r1 c1 b-list)
                                        (foo r1 c1 (cons (vector b1 r c (pay-fn b1)) b-list))))))))
   (bds (foo 7 7 '() )))
   (if (null? bds) (list (vector b 8 8 (* -1000 color))) (firstN (sort! bds (vector-ref comps (1+ color)))))))

;;; An alternate version of gen-kids. The performance of each is pretty
;;;  close to that of the other.  Left in from testing.
(define (gen-kids1 b color)
  (let ((b-list '()))
   (letrec ((foo (lambda (r c)
             (cond ((= r -1) '())
                   (#T (let ((b1 (new-board b color r c)))
                         (cond ((= c 0) (if (null? b1) (foo (-1+ r) 7)
                                              (begin (set! b-list (cons (vector b1 r c (pay-fn b1)) b-list)) (foo (-1+ r) 7))))
                               (#T (if (null? b1) (foo r (-1+ c))
                                              (begin (set! b-list (cons (vector b1 r c (pay-fn b1)) b-list)) (foo r (-1+ c))))))))))))
    (foo 7 7)
    (if (null? b-list) (list (vector b 8 8 (* -1000 color))) (firstN (sort! b-list (vector-ref comps (1+ color))))))))

;;; These are the comparison predicates sent into PC-Scheme's sort! function
;;;  during generation of children boards.  The payoff values for the
;;;  children boards are what is being compared here.
(define comps (vector (lambda (a b)
                       (< (vector-ref a 3) (vector-ref b 3)))
                      '()
                      (lambda (a b)
                       (> (vector-ref a 3) (vector-ref b 3)))))

;;; This is the static evaluator function used throughout the whole game
;;;  with the exception of the last few moves.  End-payoff is used in the
;;;  endgame.
(define (payoff3 b)
  (letrec ((foo (lambda (r c acc)
	    (cond
	     ((= r -1) (- acc 64))
	     ((= c 0) (foo (-1+ r) 7 (+ acc (modulo (quotient (vector-ref b r)
							      (expt 3 c)) 3))))
      	     (#T (foo r (-1+ c) (+ acc (modulo (quotient (vector-ref b r)
					                 (expt 3 c)) 3)))))))
           (corners (vector 0 0 7 7 0 7 7 0))
           (Pcorners (vector 1 1 6 6 1 6 6 1))
           (sides1  (vector 0 1 7 6 0 6 7 1))
           (sides2  (vector 1 0 6 7 1 7 6 0)))
  (if (< *MOVES* (- 60 *D-BOUND*))
   (+
   (do ((acc 0) (x 0 (1+ (1+ x))))
    ((= x 8) acc)
     (let* ((ccr1 (vector-ref corners x))
            (ccc1 (vector-ref corners (1+ x)))
            (pcr1 (vector-ref Pcorners x))
            (pcc1 (vector-ref Pcorners (1+ x)))
            (sr1 (vector-ref sides1 x))
            (sc1 (vector-ref sides1 (1+ x)))
            (sr2 (vector-ref sides2 x))
            (sc2 (vector-ref sides2 (1+ x)))
            (s1 (-1+ (modulo (quotient (vector-ref b sr1)
                                        (expt 3 sc1)) 3)))
            (s2 (-1+ (modulo (quotient (vector-ref b sr2)
                                        (expt 3 sc2)) 3)))
            (cc (-1+ (modulo (quotient (vector-ref b ccr1)
                                        (expt 3 ccc1)) 3)))
            (pc (-1+ (modulo (quotient (vector-ref b pcr1)
                                        (expt 3 pcc1)) 3))))
      (if (= 0 cc) (set! acc (- acc (* 10 (+ pc s1 s2))))
                   (set! acc (+ acc (* 50 cc))))))
   (foo 7 7 0))
  (foo 7 7 0))))

;;; This payoff function is used at the very end of the game when each
;;;  side's number of pieces is the only thing that matters.
(define (end-payoff b r c acc)
  (cond
    ((= r -1) (- acc 64))
    ((= c 0) (end-payoff b (-1+ r) 7 (+ acc (modulo (quotient (vector-ref b r)
				   	                      (expt 3 c)) 3))))
    (#T (end-payoff b r (-1+ c) (+ acc (modulo (quotient (vector-ref b r)
						         (expt 3 c)) 3))))))

(define (game-done)
  (writeln)
  (print-board globoard)
  (writeln)
  (display "Game Over....")
  (let ((tally (end-payoff globoard 7 7 0)))
   (cond ((= 0 tally) (writeln "It's a Draw!"))
         ((< tally 0) (writeln "I win!"))
         ((> tally 0) (writeln "You win!"))))
  (writeln))

;;; Return a new vector that is a copy of the vector v.
(define (v-copy v)
  (*v-copy (make-vector (vector-length v)) v (vector-length v) 0))

(define (*v-copy v1 v len n)
 (cond ((= len n) v1)
       (#T (vector-set! v1 n (vector-ref v n)) (*v-copy v1 v len (1+ n)))))

;;; Return the first *N* elements of a list L.
(define (firstN L)
 (letrec ((foo (lambda (LL n)
                (cond ((or (= n 0) (null? LL)) '())
                      (#T (cons (car LL) (foo (cdr LL) (-1+ n))))))))
  (foo L *N*)))

(define (print-board board)
 (newline)
 (display " ")
 (do ((i 0 (1+ i)))
     ((= i 8))
    (display " ")
    (display i))
 (newline)
 (do ((r 0 (1+ r)))
     ((= r 8))
      (display r)
      (display " ")
      (do ((c 0 (1+ c)))
  	  ((= c 8))
	   (display (vector-ref symbols (1+ (* white (square board r c)))))
	   (display " "))
      (display r)
      (newline))
 (display " ")
 (do ((i 0 (1+ i)))
     ((= i 8))
     (display " ")
     (display i))
 (newline)
 (writeln "Your piece margin: " (end-payoff globoard 7 7 0)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; The following are functions used for testing during development.

;;; Set x random squares on the board to random states.
(define (set-some x)
  (cond ((= x 0) '())
        (#T (set-square globoard (-1+ (random 3)) (random 8) (random 8))
            (set-some (-1+ x)))))

(randomize 2332)

;;; Compute (test-fn) n times and return the average time in hundredths
;;;  of a second.
(define (test1 n)
 (let ((acc 0))
  (do ((x n (-1+ x)))
      ((= 0 x) (/ acc n))
       (let ((t1 (runtime)) (t2 (test-fn)) (t2 (runtime)))
        (set! acc (+ acc (- t2 t1)))))))

;;; Show the children boards of board b after color's move.
(define (show-kids b color)
  (letrec ((foo (lambda (x)
                  (cond ((null? x) '())
                        (#T (print-board (vector-ref (car x) 0)) (foo (cdr x)))))))
   (foo (gen-kids b color))))
