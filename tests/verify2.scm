(define (test expr expected)
  (let ((result (eval-cps expr)))
    (if (equal? result expected)
        (begin
          (display "OK: ") (display result) (newline))
        (begin
          (display expr)
          (display "ERROR: expected ") (display expected)
          (display ", but got ") (display result) (newline)))))



(test (substring "hello" 0 5) "hello")
(test (substring "hello" 0 0) "")
(test (substring "hello" 2 4) "ll")
(test (substring "hello" 4 5) "o")
(test (substring "" 0 0) "")
(test (substring "abc" 0 1) "a")
(test (substring "abc" 1 3) "bc")
(test (substring "abc" 2 2) "")

(test (floor 3.7) 3)
(test (floor -3.7) -4)
(test (floor 0.0) 0)
(test (floor 5.0) 5)
(test (floor 2.0) 2)

(test (ceiling 3.2) 4)
(test (ceiling -3.2) -3)
(test (ceiling 0.0) 0)
(test (ceiling 5.0) 5)
(test (ceiling -2.0) -2)

(test (truncate 3.7) 3)
(test (truncate -3.7) -3)
(test (truncate 0.0) 0)
(test (truncate 5.0) 5)
(test (truncate -2.0) -2)

(test (round 3.2) 3)
(test (round 3.7) 4)
(test (round -3.2) -3)
(test (round -3.7) -4)
(test (round 2.5) 2)  ; even direct
(test (round 3.5) 4)  ; even direct
(test (round -2.5) -2) ; even direct
(test (round -3.5) -4) ; even direct
(test (round 0.0) 0)
(test (round 5.0) 5)


(test (quotient 7 2) 3)
(test (quotient 10 5) 2)
(test (quotient 0 5) 0)
(test (quotient 5 1) 5)
(test (quotient 5 5) 1)
(test (quotient -7 2) -3)
(test (quotient 7 -2) -3)
(test (quotient -7 -2) 3)
(test (quotient -10 5) -2)
(test (quotient 10 -5) -2)
(test (quotient -10 -5) 2)
(test (quotient 8 3) 2)
(test (quotient -8 3) -2)
(test (quotient 8 -3) -2)
(test (quotient -8 -3) 2)

(test (number->string 42)    "42")
(test (number->string -42)   "-42")
(test (number->string 0)     "0")
(test (number->string 2.5)   "2.5")
(test (number->string -0.001) "-0.001")
(test (number->string 0.0)   "0.0")  ;depend implementation

(test (string->number "42")    42)
(test (string->number "-42")   -42)
(test (string->number "+42")   42)
(test (string->number "003")   3)     
(test (string->number "2.5")   2.5)
(test (string->number "-0.001") -0.001)
(test (string->number "1e3")   1000)  
(test (string->number "1e-2")  0.01)

(test (string->number "abc")   #f)
(test (string->number "")      #f)

 ;(test (string->number " 42") 42) ;depend implementation
 ;(test (string->number "42 ") 42) ;depend implementation


(test '(symbol? 'a) #t)
(test '(symbol? 'foo) #t)
(test '(symbol? "foo") #f)
(test '(symbol? 123) #f)
(test '(symbol? #t) #f)
(test '(symbol? '()) #f)  ;; 空リストはシンボルではない

(test '(number? 123) #t)
(test '(number? -12.3) #t)
(test '(number? +0) #t)
(test '(number? 'a) #f)
(test '(number? "123") #f)

(test '(string? "abc") #t)
(test '(string? "") #t)
(test '(string? 'abc) #f)
(test '(string? 123) #f)

(test '(boolean? #t) #t)
(test '(boolean? #f) #t)
(test '(boolean? 0) #f)
(test '(boolean? '()) #f)

(test '(pair? '(1 . 2)) #t)
(test '(pair? '(1 2 3)) #t)
(test '(pair? '()) #f)
(test '(pair? 123) #f)

(test '(list? '()) #t)
(test '(list? '(1 2 3)) #t)
(test '(list? '(1 . 2)) #f)
(test '(list? 123) #f)

(test '(null? '()) #t)
(test '(null? '(1 2 3)) #f)
(test '(null? 0) #f)