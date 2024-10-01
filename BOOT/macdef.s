;;; MACDEF.S -- Macros for let, named-let, let*, unless, when, cond
;;;
;;; * Notice that I don't use quasiquote.  I would like to implement
;;;	quasiquote as a macro instead of a special form.
;;;
;;; * The cond macro must be first since other expander functions use
;;;	the cond special form.
;;;
;;; * The function ``simple-map'' must already be defined.
;;;
;;; * The expander functions are defined as separate functions so they
;;;	will be compiled.  If the expander functions were anonymous
;;;	lambda's they wouldn't be compiled.  Eg, this causes the expander
;;;	function for ``unless'' to be interpreted rather than compiled.
;;;
;;;	(macro unless
;;;	   (lambda (e) ...))
;;;

(macro cond expand-cond)

(macro let expand-let)

(macro let* expand-let*)

(macro letrec expand-letrec)

(macro unless expand-unless)

(macro when expand-when)
