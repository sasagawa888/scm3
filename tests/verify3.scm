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


(test '(symbol? '()) #f)     
(test '(symbol? 't) #t)      
(test '(symbol? 'f) #t)      
(test '(symbol? 'long-symbol-name-with-hyphens) #t)


(test '(number? 0) #t)
(test '(number? -0) #t)
(test '(number? +0) #t)
(test '(number? 'a) #f)
(test '(number? "123") #f)

(test '(string? "") #t)                     
(test '(string? "a") #t)                  
(test '(string? 'abc) #f)
(test '(string? 123) #f)


(test '(boolean? #t) #t)
(test '(boolean? #f) #t)
(test '(boolean? 0) #f)
(test '(boolean? '()) #f)

(test '(pair? '(1 . 2)) #t)     
(test '(pair? '(1 2 3)) #t)      
(test '(pair? '()) #f)          
(test '(pair? 'a) #f)

(test '(list? '()) #t)
(test '(list? '(1 2 3)) #t)
(test '(list? '(1 . 2)) #f)     
(test '(list? '((((()))))) #t)   
(test '(list? 'a) #f)

(test '(null? '()) #t)
(test '(null? '(1)) #f)
(test '(null? 0) #f)