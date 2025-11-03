
(define fact
    (lambda (n)
        (if (zero? n)
            1
            (* n (fact (- n 1))))))

(define rightmost 
    (lambda (l)
        (if (null? (cdr l))
            (car l)
            (rightmost (cdr l)))))
