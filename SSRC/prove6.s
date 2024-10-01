
;;; The Theorem-Prover.  Source version #: 6
;;;    This is the 
;;;		- iterative Scheme version 
;;;		- uses continuations.
;;;		- uses Unify
;;;		- on success, it prints only those bindings in the
;;;		  original goal
;;;
;;; The format of our knowledge base is:
;;;     ( (rule) (rule) ... )
;;;
;;;  where rule = ( consequence antec ), or
;;;        rule = ( consequence )             ; actually just a fact

(define NEXT ())

;;; (Rest Rule) returns the rest of the rule which is passed.
(define (Rest Rule)
  (cdr Rule)
)

;;; (Body Rule) returns the body of the rule which is passed.
 (define (Body Rule)
   (cdr Rule)
 )

;;; (First Rule) returns the first goal of the passed rule.
 (define (First Rule)
   (car Rule)
 )

;;; (Pertains Goal Db) returns a restricted database - a database containing
;;;   only those rules which pertain to the goal.
(define (Pertains Goal Db) Db)

;;; (Add-Number Variable Num) appends the number to the variable name.

(define (Add-Number Variable Num)
  (if (eqv? (length Variable) 3)
      Variable
      (Append Variable (list Num))
      )
)

;;; (Number Rule Num) returns the rule with all of the variable names with the
;;;    Num post-fixed.

(define (Number Rule Num)
  (if (or (null? Rule) (atom? Rule)) Rule
      (let
	  ( (FRule          (First Rule) )
	    (RRule          (Rest Rule)  ) )
	(cond
	 ( (is-var? FRule)    (cons (Add-Number FRule Num)
				    (Number RRule Num)) )

	 ( (pair? FRule)      (cons (Number FRule Num)
				    (Number RRule Num)) )
	 
	 ( #T                 (cons FRule
				    (Number RRule Num)) )
	 )
	)
      )
  )

;;; (Prover Goal Db Succ Fail NUm Bindings) tries to prove Goal from the Db.
;;; If it can prove the goal, Prove calls Succ otherwise it calls Fail.

 (define (Prover Goal Db Succ Fail Num Bindings)
   (debug-writeln "Entry to prover:")
   (debug-writeln "Goal:" Goal)
   (debug-writeln "Db  :" Db)
   (debug-writeln "Num :" Num)
   (debug-writeln "Bindings: " Bindings)

   (P* Goal (Pertains Goal Db) Db Succ Fail Num Bindings)
 )

;;; (P* Goal RDb Succ Fail Num Bindings) is the driving routine behind 
;;; (Prove).  However, it proves the rules in the restricted database.  The 
;;; restricted database are those rules out of the Db which pertain to the 
;;; Goal.

 (define (P* Goal RDb Db Succ Fail Num Bindings)

   (debug-writeln "Entry of p*")
   (debug-writeln "    Goal:" Goal)
   (debug-writeln "    RDB :" RDB)
   (debug-writeln "    Num :" Num)
   (debug-writeln "    Bindings:" Bindings)

   (if (null? RDb) (fail)
       (let*
	   ( (FRule	(First RDB))
	     (tFrule	(Number FRule Num) )
	     (tBindings	(Unify Goal (First tFRule) Bindings)) )

;	 (debug-writeln "    FRule :" FRule)
;	 (debug-writeln "    tFRule:" tFRule)
;	 (debug-writeln "    tbindings:" tBindings)
	 
	 (if (eqv? tBindings 'fail)
	 	     
	     (P* Goal
		 (Rest RDb)
		 Db 
		 Succ 
		 Fail 
		 Num 
		 Bindings)

	     (begin
	       (debug-writeln "Trying" FRule)
	       (Prove-List Goal 
			   (Body tFRule)
			   Db 
			   succ
			   (lambda () 
			     (P* Goal
				 (Rest RDb) 
				 Db 
				 Succ
				 Fail 
				 Num
				 Bindings))
			   (+ 1 Num)
			   tBindings
			   )
	       )	
	     )
	 )
       )
   )

;;; (Prove-List Goals Db Succ Fail Num Bindings) proves the Goals in a rule.
;;; If it succeeds, it calls Succ otherwise it calls Fail.

 (define (Prove-List Goal Goals Db Succ Fail Num Bindings)

   (debug-writeln "Entry to prove-list:")
   (debug-writeln "Db  :" Db)
   (debug-writeln "Num :" Num)
   (debug-writeln "Bindings: " Bindings)

   (if (null? Goals) (succ Bindings Goal fail)
     (Prover (First Goals)
	    Db
	    (lambda (result 'junk Re-Try)
	         (Prove-List Goal (Rest Goals) Db Succ Re-Try Num result))
	    Fail
	    Num
	    Bindings
     )
   )
 )

;;; (Write-Var Name Var) outputs the Var's value.
(define (Write-Var Name Var)
  (writeln Name '= Var)
)

;;; (Print-Bindings Goal Bindings) prints the values for the variables in
;;;    the Goal.
(define (Print-Bindings Goal Bindings)
  (cond
   ( (null? Goal)            #T )
   ( (is-var? (First Goal))  (begin
			       (Write-Var (First Goal) 
					  (Value (First Goal) Bindings))
			       (Print-Bindings (Rest Goal) Bindings)) )

   ( #T                      (Print-Bindings (Rest Goal) Bindings))
  )
)

;;; (Prove Goal Db) is a sample function which uses (Succ) and (Fail) to drive
;;;    the prover.  Prove needs the Goal to prove and the Db to use in proving
;;;    in this goal.  Prove keeps track of whether or not to START proving or
;;;    CONTINUE where we left off before.

(define (Prove Goal Db)
  (call/cc
	(lambda (top)
	   (prover goal

		   db

		   (lambda (result goal re-try) ; what to invoke for success
		     (set! NEXT Re-Try)
		     (Print-Bindings Goal Result)
		     (top 'Yes))

		   (lambda ()                ; what to invoke for failure
		     (writeln "Failed.")
		     (top '()))

		   0                         ; original re-number #
		   ()                        ; original bindings
	    )
	)
  )
)
