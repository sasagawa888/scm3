
(define (foo)
    (call/cc (lambda (c) (setq a c))) (bar))

(define (bar)
    (print 3))

(define boo (lambda (x) x))
