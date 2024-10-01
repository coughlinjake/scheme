(symbol? 1)
(symbol? 1.0)
(symbol? 'aaa)

(atom? 1)
(atom? 1.0)
(atom? 'aaa)

(number? 'aaa)

(zero? 'aaa)
(integer? 'aaa)
(float? 'aaa)

(eq? 'aaa 'aaa)
(eqv? 'aaa 'aaa)
(equal? 'aaa 'aaa)

(eq? 'symbol4 'symbol5)
(eqv? 'symbol4 'symbol5)
(equal? 'symbol4 'symbol5)

(eq? '(a b c) '(a b c))
(eqv? '(a b c) '(a b c))
(equal? '(a b c) '(a b c))

(equal? '(a . b) '(a b))
(equal? '(a b c (d . e)) '(a b c (d . e)))
(equal? '( () () () ) '( () () () ) )
(equal? '( () () ) '( () ) )

(pair? ())
(pair? 'a)
(pair? '(a . b))
(pair? '(a b))

(exit)
