;;; Fact.s -- compute the factorial of a number using a named let.
(define (Factorial n)
   (let fact ([i n])
      (if (zero? i)
          1
          (* i (fact (- i 1)))
      )
   )
)
