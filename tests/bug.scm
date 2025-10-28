
(define (sublists list)
    (let ((y '())')
        (do ((x list (cdr x)))
            ((null? x) y)
            (set! y (cons x y)))))