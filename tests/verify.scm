;; ==============================
;; Test Framework
;; ==============================
(define (test expr expected)
  (let ((result (eval-cps expr)))
    (if (equal? result expected)
        (begin
          (display "OK: ") (display result) (newline))
        (begin
          (display "ERROR: expected ") (display expected)
          (display ", but got ") (display result) (newline)))))

(test '(eq? 'a 'a) #t)
(test '(eq? 'a 'b) #f)
(test '(equal? '(1 2) '(1 2)) #t)
(test '(= 3 3) #t)
(test '(< 2 3) #t)
(test '(> 5 2) #t)

(test '(+ 1 2) 3)
(test '(* 2 3) 6)
(test '(- 10 4) 6)
(test '(/ 8 2) 4)
(test '(remainder 7 3) 1)
(test '(abs -5) 5)

(test '(list? '(1 2 3)) #t)
(test '(null? '()) #t)
(test '(car '(1 2 3)) 1)
(test '(cdr '(1 2 3)) '(2 3))
(test '(cons 1 '(2 3)) '(1 2 3))
(test '(length '(1 2 3 4)) 4)

(test '(and #t #t) #t)
(test '(and #t #f) #f)
(test '(or #f #t) #t)
(test '(not #t) #f)
(test '(not #f) #t)

(test '(if #t 1 2) 1)
(test '(if #f 1 2) 2)
(test '(cond ((> 3 5) 'no) ((< 3 5) 'yes)) 'yes)

(test '(let ((x 2) (y 3)) (+ x y)) 5)
(test '(let* ((x 2) (y (+ x 3))) y) 5)

(test '((lambda (x) (+ x 1)) 4) 5)
(test '(apply + '(1 2 3 4)) 10)
(test '(map (lambda (x) (* x x)) '(1 2 3)) '(1 4 9))

(test '(letrec ((even? (lambda (n) (if (= n 0) #t (odd? (- n 1)))))
                (odd? (lambda (n) (if (= n 0) #f (even? (- n 1))))))
          (even? 4)) #t)
(test '(letrec ((even? (lambda (n) (if (= n 0) #t (odd? (- n 1)))))
                (odd? (lambda (n) (if (= n 0) #f (even? (- n 1))))))
          (odd? 5)) #t)


;; ==== 文字テスト ====
(test '(char=? #\a #\a) #t)
(test '(char=? #\a #\b) #f)
(test '(char<? #\a #\b) #t)
(test '(char>? #\z #\y) #t)
(test '(char<=? #\a #\a) #t)
(test '(char>=? #\b #\a) #t)
(test '(char-ci=? #\A #\a) #t)
;(test '(char-ci<? #\A #\b) #t)


(test '(string=? "abc" "abc") #t)
(test '(string=? "abc" "abd") #f)
(test '(string<? "abc" "abd") #t)
(test '(string>? "abc" "abb") #t)
(test '(string<=? "abc" "abc") #t)
(test '(string>=? "abc" "abb") #t)
;(test '(string-ci=? "AbC" "aBc") #t)
;(test '(string-ci<? "AbC" "aBd") #t)

(test '(string-ref "abc" 0) #\a)
(test '(string-ref "abc" 2) #\c)
(test '(string-length "hello") 5)
;(test '(string-append "ab" "cd") "abcd")
