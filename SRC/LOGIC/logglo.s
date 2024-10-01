;;; logglo.s -- Global definitions for the logic engine
;;;
;;; Copyright (C) 1989 Perry and Coughlin
;;; Written at Clarkson University as a project
;;; under the direction of Levin & Searleman.

(define (cadddr l) (car (cdr (cdr (cdr l)))))
(define print display)
(define debug-on ())
(define (writeln . x) (display x) (newline))
(define (debug-writeln . x) (if debug-on (writeln x) ) )
