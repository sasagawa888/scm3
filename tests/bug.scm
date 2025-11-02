
(define (foo) 
    (let loop ((n 0))
          (display n)
          (if (= n 10)
              #t
              (loop (+ n 1)))))