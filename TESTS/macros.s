`(+ 2 3)
`(+ 2 ,(* 3 4))
`(a b (,(+ 2 3) c) d)
`(a b ,(reverse! '(c d e)) f g)
`(+ ,@(cdr '(* 2 3)))
`(a b ,@(reverse! '(c d e)) f g)
'`,(cons 'a 'b)
`',(cons 'a 'b)
`((+ 1 2) ,(+ 1 2) ,(list 1 2 (+ 1 2)) ,@(list 1 2 (+ 1 2)) a b)
(exit)
