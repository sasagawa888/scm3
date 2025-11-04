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

(test '(+) 0)
(test '(*) 1)
(test '(gcd) 0)
(test '(lcm) 1)
(test '(catch-error '(-)) 20)
(test '(catch-error '(/)) 20)

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
(test '(catch-error '(string-ref #(a b c) 3)) 3)
(test '(catch-error '(string-ref "asdf" -1)) 21)
(test '(catch-error '(string-ref "asdf" 4)) 21)

;; arity mismatch
(test '(catch-error '(if 1 2 3 4)) 20)
(test '(catch-error '(if 1)) 20)
(test '(catch-error '(modulo 1)) 12)
(test '(catch-error '(modulo 2 3 4)) 12)
(test '(catch-error '(remainder 1)) 12)
(test '(catch-error '(remainder 2 3 4)) 12)


;; --- arity mismatch ---
(test '(catch-error '(if 1 2 3 4)) 20)
(test '(catch-error '(if 1)) 20)

(test '(catch-error '(car)) 12)
(test '(catch-error '(car 1 2)) 12)
(test '(catch-error '(cdr)) 12)
(test '(catch-error '(cdr 1 2)) 12)
(test '(catch-error '(cons 1)) 12)
(test '(catch-error '(cons 1 2 3)) 12)

(test '(catch-error '(list-ref '(a b c))) 12)
(test '(catch-error '(list-ref '(a b c) 1 2)) 12)
(test '(catch-error '(list-tail '(a b c))) 12)
(test '(catch-error '(list-tail '(a b c) 1 2)) 12)

(test '(catch-error '(abs)) 11)
(test '(catch-error '(abs 1 2)) 11)
(test '(catch-error '(sqrt)) 11)
(test '(catch-error '(sqrt 9 10)) 11)
(test '(catch-error '(exp)) 11)
(test '(catch-error '(exp 1 2)) 11)
(test '(catch-error '(log)) 11)
(test '(catch-error '(log 10 2 3)) 11)
(test '(catch-error '(sin)) 11)
(test '(catch-error '(sin 1 2)) 11)
(test '(catch-error '(cos)) 11)
(test '(catch-error '(cos 1 2)) 11)
(test '(catch-error '(tan)) 11)
(test '(catch-error '(tan 1 2)) 11)
(test '(catch-error '(asin)) 11)
(test '(catch-error '(asin 1 2)) 11)
(test '(catch-error '(acos)) 11)
(test '(catch-error '(acos 1 2)) 11)
(test '(catch-error '(atan)) 20)
(test '(catch-error '(atan 1 2 3)) 20)
(test '(catch-error '(max)) 12)
(test '(catch-error '(min)) 12)

(test '(catch-error '(=)) 12)
(test '(catch-error '(<)) 12)
(test '(catch-error '(>)) 12)
(test '(catch-error '(<=)) 12)
(test '(catch-error '(>=)) 12)

(test '(catch-error '(boolean?)) 11)
(test '(catch-error '(boolean? #t #f)) 11)
(test '(catch-error '(pair?)) 11)
(test '(catch-error '(pair? '(1) 2)) 11)
(test '(catch-error '(symbol?)) 11)
(test '(catch-error '(symbol? 'a 'b)) 11)
(test '(catch-error '(null?)) 11)
(test '(catch-error '(null? '() 1)) 11)
(test '(catch-error '(number?)) 11)
(test '(catch-error '(number? 1 2)) 11)
(test '(catch-error '(string?)) 11)
(test '(catch-error '(string? "a" "b")) 11)
(test '(catch-error '(vector?)) 11)
(test '(catch-error '(vector? '#(1 2) '#(3 4))) 11)
(test '(catch-error '(procedure?)) 11)
(test '(catch-error '(procedure? + -)) 11)

(test '(catch-error '(eq?)) 12)
(test '(catch-error '(eq? 1)) 12)
(test '(catch-error '(eq? 1 2 3)) 12)
(test '(catch-error '(eqv?)) 12)
(test '(catch-error '(eqv? 1)) 12)
(test '(catch-error '(eqv? 1 2 3)) 12)
(test '(catch-error '(equal?)) 12)
(test '(catch-error '(equal? '(a))) 12)
(test '(catch-error '(equal? '(a) '(b) '(c))) 12)

(test '(catch-error '(string-length)) 11)
(test '(catch-error '(string-length "abc" "def")) 11)
(test '(catch-error '(string-ref "abc")) 12)
(test '(catch-error '(string-ref "abc" 1 2)) 12)

(test '(catch-error '(vector-length)) 11)
(test '(catch-error '(vector-length '#(1 2) '#(3))) 11)
(test '(catch-error '(vector-ref '#(a b c))) 12)
(test '(catch-error '(vector-ref '#(a b c) 1 2)) 12)

(test '(catch-error '(number->string)) 11)
(test '(catch-error '(number->string 123 10 20)) 11)
(test '(catch-error '(string->number)) 11)
(test '(catch-error '(string->number "123" 10 20)) 11)
(test '(catch-error '(symbol->string)) 11)
(test '(catch-error '(symbol->string 'a 'b)) 11)
(test '(catch-error '(string->symbol)) 11)
(test '(catch-error '(string->symbol "a" "b")) 11)


;; --- type mismatch ---
(test '(catch-error '(+ "abc" 1)) 4)
(test '(catch-error '(string-length 42)) 3)
(test '(catch-error '(string-ref 42 1)) 3)
(test '(catch-error '(vector-ref 123 0)) 7)
(test '(catch-error '(eq? "a" 1)) #f)

;; --- undefined variable ---
;(test '(catch-error 'x) 15)
;(test '(catch-error '(+ a b)) 15)


;; --- syntax error ---
(test '(catch-error '(lambda x x)) 5)
(test '(catch-error '(define (foo x y))) 22)
(test '(catch-error '(begin 1 . 2)) 22)

;; --- normal cases (no error) ---
(test '(catch-error '(substring "abcd" 1 3)) "bc")
(test '(catch-error '(vector-ref #(1 2 3) 2)) 3)
(test '(catch-error '(if #t 1 2)) 1)

