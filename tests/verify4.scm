;; Test Framework
;; ==============================
(define (test expr expected)
  (let ((result (eval-cps expr)))
    (if (equal? result expected)
        (begin
          (display "OK: ") (display result) (newline))
        (begin
          (display expr)
          (display "ERROR: expected ") (display expected)
          (display ", but got ") (display result) (newline)))))


;; normal
(test '(catch-error '(+ 1 2)) 3)
(test '(catch-error '(* 3 4)) 12)
(test '(catch-error '(- 10 7)) 3)

;; division zero
(test '(catch-error '(/ 1 0)) 19)
(test '(catch-error '(quotient 1 0)) 19)
(test '(catch-error '(modulo 1 0)) 19)
(test '(catch-error '(remainder 1 0)) 19)

;; type error not number
(test '(catch-error '(+ 1 'a)) 4)
(test '(catch-error '(- 'a 2)) 4)
(test '(catch-error '(* 1 'b)) 4)
(test '(catch-error '(/ 'x 1)) 4)

;; undefined
(test '(catch-error '(foobar 1 2)) 18)
(test '(catch-error '(undefined-func 5)) 18)

;; wrong argument
(test '(catch-error '(make-string -1)) 21)
(test '(catch-error '(substring "asdf" -1 2)) 21)
(test '(catch-error '(substring "asdf" 1 -2)) 21)
(test '(catch-error '(substring "asdf" 1 5)) 21)
(test '(catch-error '(substring "asdf" 1 4)) "sdf")
(test '(catch-error '(vector-ref "abc" 1)) 7)
(test '(catch-error '(vector-ref #(a b c) -1)) 21)
(test '(catch-error '(vector-ref #(a b c) 3)) 21)


;; arity mismatch
(test '(catch-error '(if 1 2 3 4)) 20)
(test '(catch-error '(if 1)) 20)


;; --- arity mismatch ---
(test '(catch-error '(if 1 2 3 4)) 20)
(test '(catch-error '(if 1)) 20)
(test '(catch-error '(car)) 11)
(test '(catch-error '(cdr 1 2)) 11)
(test '(catch-error '(cons 1 2 3)) 12)

;; --- type mismatch ---
(test '(catch-error '(+ "abc" 1)) 4)
(test '(catch-error '(string-length 42)) 3)
(test '(catch-error '(vector-ref 123 0)) 7)
(test '(catch-error '(eq? "a" 1)) #f)

;; --- undefined variable ---
(test '(catch-error 'x) 15)
;(test '(catch-error '(+ a b)) 15)

;; --- division by zero ---
(test '(catch-error '(/ 1 0)) 19)

;; --- syntax error ---
(test '(catch-error '(lambda x x)) 5)
;(test '(catch-error '(define (foo x y))) 24)
;(test '(catch-error '(begin 1 . 2)) 24)

;; --- normal cases (no error) ---
(test '(catch-error '(substring "abcd" 1 3)) "bc")
(test '(catch-error '(vector-ref #(1 2 3) 2)) 3)
(test '(catch-error '(if #t 1 2)) 1)

