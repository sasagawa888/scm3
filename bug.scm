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

(define (tarai x y z)
  (if (<= x y)
      y
    (tarai (tarai (- x 1) y z) (tarai (- y 1) z x) (tarai (- z 1) x y))))

;;(define f nil)
;;(+ 1 2 (call/cc (lambda (c) (set! f c))))
