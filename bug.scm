
(define (foo x)
    (letrec ((even? (lambda (n)
                      (display n)
                      (if (= n 0)
                          #t
                          (odd? (- n 1)))))
             (odd? (lambda (n)
                      (display n)
                      (if (= n 0)
                          #f
                          (even? (- n 1))))))
    (even? x)))


(define (test x y)
    (cond ((not (equal? x y)) 
           (display "error") (display x) (display y) (newline))
          (else #t)))