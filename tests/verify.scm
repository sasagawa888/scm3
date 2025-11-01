
;; ==============================
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

;; ==============================================================
;; Basic arithmetic and comparisons
;; ==============================================================

(test '(+ 1 2 3) 6)
(test '(- 10 4 1) 5)
(test '(* 2 3 4) 24)
;(test '(/ 8 2 2) 2)
;(test '(= 3 3 3) #t)
;(test '(< 2 3 4) #t)
;(test '(> 5 2 1) #t)
;(test '(<= 3 3 4) #t)
;(test '(>= 5 5 4) #t)

;; ==============================================================
;; Remainder / modulo precise tests
;; ==============================================================

;; remainder
(test '(remainder 5 3) 2)
(test '(remainder -5 3) -2)
(test '(remainder 5 -3) 2)
(test '(remainder -5 -3) -2)
(test '(remainder 0 3) 0)
(test '(remainder 3 1) 0)
(test '(remainder -1 3) -1)

;; modulo
(test '(modulo 5 3) 2)
(test '(modulo -5 3) 1)
(test '(modulo 5 -3) -1)
(test '(modulo -5 -3) -2)
(test '(modulo 0 3) 0)
(test '(modulo -1 3) 2)

;; ==============================================================
;; Numeric functions
;; ==============================================================

(test '(abs -5) 5)
(test '(abs 5) 5)
(test '(expt 2 3) 8)
(test '(expt 9 0.5) 3.0)
(test '(sqrt 4) 2.0)
(test '(sqrt 0) 0.0)
(test '(sin 0) 0.0)
(test '(cos 0) 1.0)
(test '(< (abs (- (tan (/ 3.141592653589793 4)) 1.0)) 1e-10) #t)
(test '(= (exp 0) 1.0) #t)
(test '(= (log 1.0) 0.0) #t)
(test '(< (abs (- (log (exp 2.0)) 2.0)) 1e-10) #t)

;; ==============================================================
;; Character and string functions
;; ==============================================================

(test '(char=? #\a #\a) #t)
(test '(char=? #\a #\b) #f)
(test '(char<? #\a #\b) #t)
(test '(char>? #\z #\y) #t)
(test '(char<=? #\a #\a) #t)
(test '(char>=? #\b #\a) #t)
(test '(char-ci=? #\A #\a) #t)
(test '(char-ci<? #\A #\b) #t)
(test '(string=? "abc" "abc") #t)
(test '(string=? "abc" "abd") #f)
(test '(string<? "abc" "abd") #t)
(test '(string>? "abc" "abb") #t)
(test '(string-ci=? "AbC" "aBc") #t)
(test '(string-ci<? "AbC" "aBd") #t)
(test '(string-length "hello") 5)
(test '(string-ref "abc" 0) #\a)
(test '(string-ref "abc" 2) #\c)
(test '(string-append "ab" "cd") "abcd")
(test '(string->list "abc") '(#\a #\b #\c))
(test '(list->string '(#\a #\b #\c)) "abc")

;; ==============================================================
;; List and pair functions
;; ==============================================================

(test '(list? '(1 2 3)) #t)
(test '(null? '()) #t)
(test '(car '(1 2 3)) 1)
(test '(cdr '(1 2 3)) '(2 3))
(test '(cons 1 '(2 3)) '(1 2 3))
(test '(length '(1 2 3 4)) 4)
(test '(reverse '(1 2 3)) '(3 2 1))
(test '(append '(1 2) '(3 4)) '(1 2 3 4))
(test '(append! '(1 2) '(3 4)) '(1 2 3 4))
(test '(map (lambda (x) (* x x)) '(1 2 3)) '(1 4 9))
(test '(for-each (lambda (x) x) '(1 2 3)) '(1 2 3))

;; ==============================================================
;; eq / eqv / equal tests
;; ==============================================================

(define x '(1 2))
(define y x)
(test '(eq? 'a 'a) #t)
(test '(eq? 'a 'b) #f)
(test '(eqv? 'a 'a) #t)
(test '(eqv? 'a 'b) #f)
(test '(eqv? 1 1) #t)
(test '(eqv? 1 1.0) #t)
(test '(eqv? x y) #t)
(test '(eqv? x '(1 2)) #f)
(test '(equal? '(1 2) '(1 2)) #t)

;; ==============================================================
;; Control structures
;; ==============================================================

(test '(and #t #t #t) #t)
(test '(and #t #f #t) #f)
(test '(or #f #f #t) #t)
(test '(not #t) #f)
(test '(not #f) #t)
(test '(if #t 1 2) 1)
(test '(if #f 1 2) 2)
(test '(cond ((> 3 5) 'no) ((< 3 5) 'yes)) 'yes)
(test '(case 2 ((1) 'a) ((2) 'b)) 'b)

;; ==============================================================
;; let / let* / letrec
;; ==============================================================

(test '(let ((x 2) (y 3)) (+ x y)) 5)
(test '(let* ((x 2) (y (+ x 3))) y) 5)
(test '(letrec ((even? (lambda (n) (if (= n 0) #t (odd? (- n 1)))))
                (odd? (lambda (n) (if (= n 0) #f (even? (- n 1))))))
          (even? 4)) #t)
(test '(letrec ((even? (lambda (n) (if (= n 0) #t (odd? (- n 1)))))
                (odd? (lambda (n) (if (= n 0) #f (even? (- n 1))))))
          (odd? 5)) #t)

;; ==============================================================
;; lambda / apply tests
;; ==============================================================

(test '((lambda (x y) (+ x y)) 3 4) 7)
(test '(apply + '(1 2 3 4)) 10)
(test '(apply * '(2 3 4)) 24)

;; ==============================================================
;; Floating-point precision tests
;; ==============================================================

(test '(< (abs (- (sin (/ 3.141592653589793 2)) 1.0)) 1e-10) #t)
(test '(< (abs (- (atan 1.0) (/ 3.141592653589793 4))) 1e-10) #t)
(test '(< (abs (- (atan 1.0 1.0) (/ 3.141592653589793 4))) 1e-10) #t)
(test '(= (atan 0.0 1.0) 0.0) #t)
(test '(= (atan 0.0 -1.0) 3.141592653589793) #t)
(test '(= (atan 1.0 0.0) (/ 3.141592653589793 2)) #t)
(test '(= (atan -1.0 0.0) (- (/ 3.141592653589793 2))) #t)

;; ==============================================================
;; Vector tests
;; ==============================================================

(define v (vector 1 2 3))
(test '(vector? v) #t)
(test '(vector-length v) 3)
(test '(vector-ref v 0) 1)
(test '(vector-ref v 2) 3)

;; ==============================================================
;; Promise / delay / force tests
;; ==============================================================

(define p (delay (+ 1 2)))
(test '(promise? p) #t)
(test '(force p) 3)
;; force again should return the same value without recomputation
(test '(force p) 3)

;; ==============================================================
;; call-with-current-continuation / call/cc tests
;; ==============================================================

;; Simple continuation example
;(define result
;  (call/cc
;    (lambda (k)
;      (k 42)
;      0)))
;(test 'result 42)

;; ==============================================================
;; More complex call/cc example
;; ==============================================================

;; Escape from inside a loop
;(define escape-test
;  (call/cc
;    (lambda (exit)
;      (for-each
;        (lambda (x)
;          (if (= x 3) (exit x)))
;        '(1 2 3 4 5))
;      0)))
;(test 'escape-test 3)

;; ==============================================================
;; Force with nested delay
;; ==============================================================

(define nested-p (delay (delay (+ 2 3))))
;; first force returns a promise
(test '(promise? (force nested-p)) #t)
;; second force returns the final value
(test '(force (force nested-p)) 5)

;; ==============================================================
;; Edge cases for delay / force
;; ==============================================================

(define zero-delay (delay 0))
(test '(force zero-delay) 0)
(define negative-delay (delay -10))
(test '(force negative-delay) -10)

;; ==============================================================
;; Summary: All added tests check core R3RS features
;; ==============================================================
