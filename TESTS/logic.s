(load "extend.s")
(load "unify2.s")
(load "prove6.s")
(load "prims.s")

(prove '(member 1 (@ 2 1 3)) memberdb)
(next)
(prove '(member (? x) (@ 1 2 3)) memberdb)
(next)
(next)
(exit)