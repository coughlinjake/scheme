/* Written 10:44 am  Apr 27, 1990 by jets@clutx.clarkson.edu in clutx.clarkson.edu:AI */
/* ---------- "Linneus.s" ---------- */

;;; Tanimoto, "The Elements of Artificial Intelligence", 
;;; Computer Science Press, 1987
;;; Chapter 4, Sections 4.5 and 4.6

;;; Linneus is a simple dialog system which remembers ISA and INCLUDES
;;; relations between categories, and answers questions about them.
;;; It is modelled on SIR (Semantic Information Retrieval), by B. Raphael.
;;;
;;; It looks for the following patterns in sentences input from the user:
;;;
;;;     (A * is a *)
;;;     (What is a *)
;;;     (Is a * a *)
;;;     (Why is a * a *)

(define (Linneus)
   (print '(I am Linneus))
   (print '(Please give me information or ask me questions))
   (newline)
   (do
      ((sentence (Read_Sentence) (Read_Sentence)))
      ((equal? sentence '(bye)) '(Goodbye))
         (newline)
         (interpret sentence)
         (newline)))

(define (Read_Sentence)
   (newline)
   (princ ">")
   (read))
;;; end Linneus


;;; Interpret is given text (a sentence in the form of a list of words)
;;; matches it against a number of simple patterns, and constructs
;;; an appropriate response

;;; First define the following global variables used by interpret
;;; (kludge introduced for compatibility with Tanimoto's version
(define article1 nil)
(define X nil)
(define article2 nil)
(define Y nil)

(define (interpret text)
   (cond
      ;; if text is of the form (An X is a Y) then
      ((match '((MatchArticle article1) (? X) IS
                (MatchArticle article2) (? Y))
              text) 
         ;; add relations X ISA Y and Y INCLUDES X,
         ;; and respond "I understand"
         (AddSuperset X Y)
         (AddSubset Y X)
         (putprop X article1 'ARTICLE)
         (putprop Y article2 'ARTICLE)
         (print '(I understand)))

      ;; if text is of the form (What is an X) then
      ((match '(What is (MatchArticle article1) (? X)) text)
         ;; look for all Yi such that either X ISA Yi or X INCLUDES Yi,
         ;; and, if any Yi's are found, respond "An X is a Y1 and a Y2 and ..."
         ;; or "An X is something more general than Y1 and Y2 and ..." 
         ;; depending on the kind of relation
         (let ((ISAflag nil)
               (IncludeFlag nil))
            (cond
               ((set! Y (getprop X 'ISA)) (set! ISAflag #t))
               ((set! Y (getprop X 'INCLUDES)) (set! IncludeFlag #t)))
            (print (append
                      (list (getprop X 'ARTICLE))
                      (list X)
                      (cond
                        (ISAflag '(is))
                        (IncludeFlag '(is something more general than)))
                      (MakeConj Y)))))

      ;; if text is of the form (Is an X a Y) then
      ((match '(Is (MatchArticle article1) (? X)
                   (MatchArticle article2) (? Y))
              text)
         ;; Search the ISA-chains rooted at X to a depth of no more than 10.  
         ;; If Y is found, respond "Yes indeed, an X is a Y".  Otherwise,
         ;; respond "Sorry, not that I know of"
         (cond
            ((ISAtest X Y 10)
               (print (append '(Yes indeed)
                              (list (getprop X 'ARTICLE))
                              (list X)
                              '(is)
                              (list (getprop Y 'ARTICLE))
                              (list Y))))
            (#t (print '(Sorry ... not that I know of)))))

      ;; if text is of the form (Why is an X a Y) then
      ((match '(Why is (MatchArticle article1) (? X)
               (MatchArticle article2) (? Y))
              text)
         ;; First check that an X is a Y by searching the ISA-chains.
         ;; If so, trace the path from X to Y.  Otherwise respond
         ;; "But it isn't!"
         (cond
            ((ISAtest X Y 10)
               (print (cons 'Because
                            (Explain_Links X Y))))
            (#t (print '(But it is not one!)))))

      ;; If text does not match any of the above forms, 
      ;; respond "I do not understand"
      (#t (print '(I do not understand)))))
;;; end Interpret

;;; Return t if X is an article of speech, nil otherwise
(define (MatchArticle X)
   (member X '(a an the that this those these)))

;;; Given a list of categories, Lst, construct a phrase by
;;; preceeding each category by its corresponding article,
;;; and inserting the conjunction "and" in between each one
(define (MakeConj Lst)
   (cond
      ((null? Lst) nil)
      ((null? (cdr Lst)) (cons (getprop (car Lst) 'ARTICLE) Lst))
      (#t (cons (getprop (car Lst) 'ARTICLE)
                (cons (car Lst)
                      (cons 'and (MakeConj (cdr Lst))))))))

;;; Given that X is a Y, trace through the path of ISA relations
;;; that link X to Y, say X ISA p1 ISA p2 ISA ... pk ISA Y,
;;; describing each intermediate node on the path
(define (Explain_Links X Y)
   (cond
      ;; if X is the same as Y then
      ;;    respond "They are identical"
      ((eq? X Y) '(They are identical))

      ;; If X ISA Y then
      ;;    respond "You told me so"
      ((member Y (getprop X 'ISA)) '(You told me so))

      ;; otherwise, trace through the chain of ISA links,
      ;; responding "X is a P1 and P1 is a P2 and ... and Pk is a Y"
      (#t (Explain_Chain X (getprop X 'ISA) Y))))

(define (Explain_Chain X L Y)
   (cond
      ((null? L) nil)
      ((member Y L) (cons 'AND (tell X Y)))
      ((ISAtest (car L) Y 10)
         (append (tell X (car L))
                 (Explain_Chain (car L) (getprop (car L) 'ISA) Y)))
      (#t (Explain_Chain X (cdr L) Y))))

(define (tell X Y)
   (list (getprop X 'ARTICLE) X 'IS (getprop Y 'ARTICLE) Y))

;;; Add element Elt to the set Set, returning the result
(define (AddToSet Elt Set)
   (cond
      ((member Elt Set) Set)
      (#t (cons Elt Set))))

;;; Add X to the ISA relation for Name
(define (AddSuperset Name X)
   (putprop Name (AddToSet X (getprop Name 'ISA)) 'ISA))

;;; Add X to the INCLUDES relation for Name
(define (AddSubset Name X)
   (putprop Name (AddToSet X (getprop Name 'INCLUDES)) 'INCLUDES))

;;; Returns t if there is an ISA chain of relations from X to Y
;;; (X ISA p1 ISA p2 ISA ... ISA Y) of length <= N; nil otherwise
(define (ISAtest X Y N)
   (cond
      ((eqv? X Y) #t)
      ((zero? N) nil)
      ((member Y (getprop X 'ISA)) #t)
      (#t (Any (mapcar
                  (lambda (P) (ISAtest P Y (- N 1)))
                  (getprop X 'ISA))))))

;;; Returns t if any member of Lst is non-nil; nil otherwise
(define (Any Lst)
   (cond
      ((null? Lst) nil)
      ((car Lst) #t)
      (#t (Any (cdr Lst)))))


;;; Tanimoto, Chapter 3, p.64   Recursive pattern-matching function

;;; match compares pattern P to subject S, succeeding if they match
;;; (and as a side-effect, setting global variables if variable patterns
;;; (? X), (* X), or (predicate X) appear in the pattern);
;;; otherwise match returns nil

(define (match P S)
   (cond

      ((null? P)  
         (null? S))

      ((atom? (car P))
         (and S 
              (equal? (car P) (car S))
              (match (cdr P) (cdr S))))

      ((and S (eq? (caar P) '?))
         (cond
            ((match (cdr P) (cdr S))
                (set (cadar P) (car S)) #t)
            (#t nil)))

      ((eq? (caar P) '*)
         (cond
            ((and S (match (cdr P) (cdr S)))
               (set (cadar P) (list (car S))) #t)

            ((match (cdr P) S)
               (set (cadar P) nil) #t)

            ((and S (match P (cdr S)))
               (set (cadar P) (cons (car S) (eval (cadar P)))) #t)

            (#t nil)))

      ((and S (eval (list (caar P) (list 'quote (car S)))) 
            (match (cdr P) (cdr S)))
          (set (cadar P) (car S)) #t)

      (#t nil)))

;;; extremely restricted form of set, which will bind values
;;; to global variables used in Linneus, if previously defined
;;; (kludge used for compatibility with Tannimoto)
(define (set var value)
   (cond
      ((equal? var 'X) (set! X value))
      ((equal? var 'Y) (set! Y value))
      ((equal? var 'article1) (set! article1 value))
      ((equal? var 'article2) (set! article2 value))
      (#t nil)))

/* End of text from clutx.clarkson.edu:AI */
