(define a 1)

(define (foo)
    (call/cc (lambda (c) (set! a c))) (bar))

(define (bar)
    (print 3))

(define boo (lambda (x) x))

(define (fact n)
    (if (= n 0)
        1
        (* n (fact (- n 1)))))

(define (fib n)
    (if (< n 2)
        n
        (+ (fib (- n 1)) (fib (- n 2)))))
