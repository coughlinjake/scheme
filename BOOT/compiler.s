;;; compiler.s - Compiler Version 1.2
;;;	written by Jason Coughlin
;;;
;;;   Macros are stored in the a-list *EXPANSION-TABLE*.  The system expands
;;; macros before evaluation and before compilation.  Compiler-syntax forms
;;; are stored in the a-list *COMPILER-SYNTAX*.  Compiler-syntax forms are
;;; expanded ONLY BEFORE COMPILATION.
;;;
;;;   Compiler-syntax forms are forms that are probably implemented in the
;;; interpreter but are not expressions that the compiler knows how to
;;; compile.  The expander expands the expression into something that the
;;; compiler CAN compile.
;;;
;;;   This expander is only used by the compiler; EVAL has its own expander.

(define *COMPILER-SYNTAX* '())

;;; (COMPILER-SYNTAX keyword function)
(macro compiler-syntax
   (lambda (e)
	(list 'add-compiler-syntax (list 'quote (cadr e)) (caddr e))
   )
)

;;; (add-compiler-syntax keyword func) - Add a compile-syntax form to
;;;	the *COMPILER-SYNTAX* table.
(define (add-compiler-syntax keyword func)
   (let ([binding	(assoc keyword *COMPILER-SYNTAX*)])
	(if (null? binding)
	   (cons (cons keyword func) *COMPILER-SYNTAX*)
	   (set-cdr! binding func)
	)
   )
)

;;; (define (name parm-list) body) == (define name (lambda parm-list body))
(compiler-syntax define
   (lambda (e)
      (if (atom? (cadr e))
	   e
	   (list 'define (caadr e) (append '(lambda) (list (cdadr e)) (cddr e)))
      )
   )
)

;;; (Expand exp) - Expand ALL macros in exp.  Macros are expanded before
;;;	compile-syntax forms.
(define (expand exp)
   (if (not (pair? exp))

	; can't expand expressions which are atoms
	exp

	; expanding this expression, pull out the expander functions
	(let* ( (macro-bind	(assoc (car exp) *EXPANSION-TABLE*) )
		(compile-bind	(assoc (car exp) *COMPILER-SYNTAX*) )
		(macro-exp	(and (not (null? macro-bind)) (cdr macro-bind)) )
		(compile-exp	(and (not (null? compile-bind)) (cdr compile-bind)) )
	      )

		(if (null? macro-exp)
		   ; there is no macro expansion, see if there is a
		   ; compile-syntax expansion to be performed.
		   (if (null? compile-exp)

		      ; no expansions, expand sub-expressions
		      (map1 expand exp)

		      ; expand sub-expressions after expanding THIS expression
		      (map1 expand (compile-exp exp))
		   )

		   ; there is a macro expansion, ignore the compiler-syntax
		   ; for now since there is no guarantee what the expression
		   ; will look like after the macro expansion.  invoke
		   ; expand again after the macro to expand a possible
		   ; compiler-syntax which results from the macro
		   (expand (macro-exp exp))
		)
	)
   )
)

;;; (COMPILE exp) - Compile expression exp.
(define (COMPILE exp)
   (*compile* (expand exp))
)

;;; (LOAD-AND-COMPILE name) - Load and compile expressions from name.
;;;(define (LOAD-AND-COMPILE name)
;;;)
