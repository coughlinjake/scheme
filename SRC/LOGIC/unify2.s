;;; Unify - Version 2
;;;
;;; Written by Jay B. Perry and Jason Coughlin.
;;;
;;; Copyright (C) 1989 Perry and Coughlin
;;; Written at Clarkson University as a project
;;; under the direction of Levin & Searleman.
;;;
;;;     Unify returns 'fail if a unification can not be made, and returns
;;; an association list of bindings if one can be made.
;;;
( define ( Unify A B Bindings )
     (cond
        ;  BASE CASE:  Bindings are equal to 'fail, return 'fail.
        ;
          ( (eqv? Bindings 'fail)       'fail )

        ;  if A and B are atoms and A = B return Bindings
        ;  else return 'fail
        ;
          ( (and (atom? A) (atom? B))   (if (eqv? A B) Bindings 'fail) )

        ;  if A is a variable, unify it with B.
        ;  if B is a variable, unify it with A.
        ;
          ( (is-var? A)                 (unify-var A B Bindings) )
          ( (is-var? B)                 (unify-var B A Bindings) )

        ;  If A and B are both of the list data type, then unify them as
        ;  such.
        ;
          ( (and (is-DTlist? A) (is-DTlist? B))  (unify-DTlist A B Bindings) )

        ;  if both functors, then unify their lists of terms.
        ;
          ( (and (pair? A) (pair? B) (eqv? (car A) (car B)))
                                     (unify-list (cdr A) (cdr B) Bindings) )

        ;  if none of the above cases apply, then the unification is not
        ;  possible, and thus fails.
        ;
          ( #T                       'fail)
     )
)


;;; unify-var
;;;
;;;     Unify-var unifies A with B.  A is assumed to be a variable.  B can
;;; be anything.
;;;
( define ( unify-var A B Bindings )
     (let ( (NameA (name A)) )
       ; If B is a variable
       ;
         (cond
            ( (is-var? B)   (let ( (NameB (name B)) )
                               (cond
                                  ; If it's the same variable, then
                                  ; just return the Bindings.
                                  ;
                                    ( (eqv? NameA NameB)   Bindings )

                                  ; If A is not bound, then bind A to B
                                  ; if it won't cause a looping in the
                                  ; bindings.
                                  ;
                                    ( (no-value? NameA Bindings)
                                            (add-new-var A B Bindings) )

                                  ; If B is not bound, then bind B to A
                                  ; if it won't cause a looping in the
                                  ; bindings.
                                  ;
                                    ( (no-value? NameB Bindings)
                                            (add-new-var B A Bindings) )

                                  ; Both variables are instantiated, so
                                  ; unify their instantiations.
                                  ;
                                    ( #T     (Unify (value A Bindings)
                                                   (value B Bindings)
                                                   Bindings) )
                               )) )

          ; is B a list? 
          ;    
            ( (is-DTlist? B)  (if (no-value? NameA Bindings)
                              ; A has no value, so bind B to A if A
                              ; is not in B (occurs-check).  If A
                              ; is in B, then fail because a binding
                              ; can not be made.
                                (if (null? (element-of? A B))
                                    (bind NameA B Bindings)
                                     'fail)

                              ; A has a value, so unify B with that value.
                                (Unify B (value A Bindings) Bindings)
                            ) )
          ; else   (B isn't a variable or a list, A is a variable)
          ;
            ( #T       (if (no-value? NameA Bindings)
                                (if (null? (element-of? A B))
                                    (bind NameA B Bindings)
                                     'fail)

                              ; A has a value, so unify B with that value.
                                (Unify B (value A Bindings) Bindings)
                            ) )
                         ; if A doesn't have a value, then bind A to B.
                         ;
;                           (bind NameA B Bindings)

                         ; Otherwise unify B with A's value.
                         ;
;                           (Unify B (value A Bindings) Bindings)
;                       ) )
          )
     )
)

;;;  no-value?
;;;
;;;       No-value? checks the association list to see if a
;;;  the name of a variable has been previously instantiated.
;;;  If so, it returns true, otherwise nil.
;;;
( define ( no-value? name Bindings)
     (null? (lookup name Bindings))
)

;;;  add-new-var
;;;
;;;    Add-new-var unifies two variables A and B by first checking to make
;;;  sure that the value of B is not equal to A.  This is to prevent a circular
;;;  definition (ie A bound to A).  If A and the value of B are not equal, then
;;;  A is bound to the value of B, otherwise the original bindings are
;;;  returned.
( define ( add-new-var A B Bindings )
     (let ( (ValueofB (value B Bindings)) )
          (if (equal? ValueofB A)
              Bindings
              (bind (name A) ValueofB Bindings)
          )
     )
)

;;; unify-DTlist
;;;
;;;     Unify-DTlist unifies lists of the list data type.  It returns
;;; the list of bindings if a unification is possible and 'fail otherwise.
;;; A and B are assumed to be of the list data type.
;;;
( define ( unify-DTlist A B Bindings )
     (cond
          ( (and (null-list? A) (null-list? B))  Bindings )
	  ( (or (null-list? A) (null-list? B))   'Fail    )
          ( (and (ht-type? A) 
                 (ht-type? B))  (Unify (ht-tail A) (ht-tail B) 
                                       (Unify (ht-head A) (ht-head B) 
                                              Bindings)) )
          ( (ht-type? A)        (Unify (ht-tail A) (new-tail B) 
                                       (Unify (ht-head A) (new-head B) 
                                              Bindings)) )
          ( (ht-type? B)        (Unify (new-tail A) (ht-tail B) 
                                       (Unify (new-head A) (ht-head B) 
                                              Bindings)) )
          ( #T                  (Unify (new-tail A) (new-tail B) 
                                       (Unify (new-head A) (new-head B) 
                                              Bindings)) )
     )
)

;;;  ht-type?
;;;
;;;     Ht-type? determines if something that is of the list data type
;;;  is of the form (@ H ! T), where H represents the head of the list
;;;  and T represents the tail of the list.  It returns true if it is
;;;  and nil otherwise.  This type of list will be referred to as an
;;;  ht-list.
;;;
( define ( ht-type? A )
     (if (>= (length A) 3)
         (eqv? (caddr A) '! )
         '())
)
         
;;;  ht-head
;;;
;;;     Ht-head returns the head of an ht-list, which will be either a
;;;  variable or an atom.
;;;
( define ( ht-head A )
     (cadr A)
)

;;;  ht-tail
;;;
;;;     Ht-tail returns the tail of an ht-list, which will be something
;;;  of the list data type or a variable.
;;;
( define ( ht-tail A )
     (cadddr A)
)

;;;  new-head
;;;
;;;     New-head returns the head of something of the list data type, which
;;;  will be an atom or a variable.  
;;; 
( define ( new-head A )
     (cadr A)
)

;;;  new-tail
;;;
;;;     New-tail returns the tail of something of the list data type, which
;;;  will be something of the list data type or a variable.
;;;
( define ( new-tail A )
     (cons '@ (cddr A))
)

;;;  element-of?
;;;
;;;     Element-of? returns the sublist of A which has as its first
;;;  element A.  If A isn't in the list, then nil is returned.  If
;;;  B isn't a list then we know that A isn't in B so return nil.
;;;
( define ( element-of? A B )
     (if (pair? B)   (member A B)
                     '())
)

;;;  null-list?
;;;
;;;      Null-list? returns true (a non-null list) if a list of the form 
;;;  (@) is found and nil otherwise.  It is a check for a null list of the 
;;;  data type list.
;;;
( define ( null-list? A )
     (if (pair? A)   (and (eqv? (car A) '@) (null? (cdr A)))
                     '())
)

;;; unify-list
;;;
;;;     Unify-list unifies the argument list of a function call.  It returns
;;; 'fail if the unification can't be made or the list of bindings otherwise.
;;;
( define ( unify-list A B Bindings )
   ;  A and B are lists.
   ;
     (cond
        ;   BASE CASE:  Bindings are equal to 'fail, return 'fail.
        ;
          ( (eqv? Bindings 'fail)    'fail )

        ;   If A and B are both null, then they are equal and thus unified.
        ; Therefore return the bindings.  If one is null but the other isn't,
        ; then they can't be unified, and thus return 'fail.
        ;
          ( (null? A)                  (if (null? B) Bindings 'fail) )
          ( (null? B)                  'fail )

        ;   Otherwise, unify the rest of the list of arguments.
          ( #T                         ( unify-list (cdr A)
                                                   (cdr B)
                                                   (Unify (car A)
                                                          (car B)
                                                          Bindings
                                                   )
                                      )
          )
     )
)

;;; lookup
;;;
;;;     Lookup returns nil if the key is not in the association list of
;;; bindings and returns a list containing the key and the value otherwise.
;;; The bindings are represented as dotted pairs of the form (key . value).
;;;
( define ( lookup key Bindings )
     (assoc key Bindings)
)

;;; name
;;;
;;;     Name returns the name of the variable A.  Variables have the form 
;;; (? x num), where x is the name of the variable.
;;;
( define ( name A )
     (cdr A)
)

;;; is-var?
;;;
;;;     Is-var? returns true if A is a variable and nil otherwise.  Variables
;;; have the form (? x num), where (x num) is the name of the variable.
;;;
( define ( is-var? A )
     (if (pair? A)   (eqv? (car A) '?)
	              '() )
)

;;; is-DTlist?
;;;
;;;     Is-DTlist returns true if A is of the list data type and nil 
;;; otherwise. A list can be distinguished as being of the list data type
;;; because its first element is the @ symbol.
;;;
( define ( is-DTlist? A)
     (if (pair? A)   (eqv? (car A) '@)
                     '())
)


;;; value
;;;
;;;     Value returns a list containing the value of the key looked up in the
;;; association list of bindings.  If A has no value in the bindings, then A
;;; is returned as its value.  If A's value is another variable, then the
;;; value that is returned is the value of that variable.  The bindings are
;;; represented as dotted pairs of the form (name . value).  The parameter A
;;; is a variable of the form (? x num), where num is any integer.
;;; 
( define ( value A Bindings )
     (let ( (BindingofA    (lookup (name A) Bindings)) )
          (if (null? BindingofA)
              A
              (if (is-var? (cdr BindingofA))
                  (value (cdr BindingofA) Bindings)
                  (cdr BindingofA)
              )
          )
     )
)

;;; bind
;;;
;;;   Takes a variable name, it's value, and the current bindings and returns
;;; the new bindings.  If the variable is already bound, it's the old bindings
;;; that are returned.
;;;
(define ( bind A Value Bindings )
       (cond
	     ; if A is already in the bindings then return the old bindings
             ;
             ( (lookup A Bindings)        Bindings )

             ; otherwise add this new binding to the current bindings
             ;
	     ( #T         (cons (cons A Value) Bindings) )
       )
)

