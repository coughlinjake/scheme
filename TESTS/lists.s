(list 'a (+ 3 4) 'c)
(list "this" "is" "a" "test")
(length '(this is a test))
(length '(this (is another) (good (for nothing)) test))
(define a '( a b c))
(define b '( d e f))
(define c '(g h i))
(define d '(a b))
(define e '(e f))
(eq? e (cddr (append d e)))
(append a b c)
(append '(x) '(y))
(append () 'a)
(append '(1 2) () 'a)
(reverse '(9 8 7 6 5 4 3 2 1 0))
(reverse '(a (b c) d (e (f))))
(memq 'a '(a b c))
(memq 'b '(a b c))
(memq 'a '(b c d))
(memq (list 'a) '(b (a) c))
(member (list 'a) '(b (a) c))
(memv 101 '(100 101 102))
(memv 101 '(100 (101) 102))
(member 101 '(100 (101) 102))
(define f '((a 1) (b 2) (c 3)))
(assq 'a f)
(assq 'b f)
(assq 'd f)
(assq (list 'a) '(((a)) ((b)) ((c))))
(assoc (list 'a) '(((a)) ((b)) ((c))))
(assv 5 '((2 3) (5 7) (11 13)))
(exit)