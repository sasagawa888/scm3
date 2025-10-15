(define a 1)

(define (foo)
    (call/cc (lambda (c) (set! a c))) (bar))

(define (bar)
    (print 3))

(define boo (lambda (x) x))
