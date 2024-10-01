(eqv? 1 1)
(eqv? 1 1.0)
(equal? 1 1)

(equal? '(1 2 3) '(1 2 3))

(atom? 1)
(atom? 1.5)

(number? 5)
(number? 5.0)

(symbol? 5)
(symbol? 5.0)

(zero? 1)
(zero? 0)
(zero? 0.0)

(integer? 1)
(integer? 1.0)
(float? 1)
(float? 1.0)

(integer? (+ 1 1.0))
(float? (+ 1 1.0))
(integer? (+ 1 4))

(< 2 5)
(> 2 5)
(< 2.0 5)
(> 2.0 5)

(< 5 2)
(> 5 2)
(< 5.0 2)
(> 5.0 2)

(= 6 6)
(= 6 6.0)
(= 6 1)

(<> 6 6)
(<> 6 6.0)
(<> 6 1)

(abs -1)
(abs -1.0)
(abs (* -1 4))
(exit)
