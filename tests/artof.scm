
(define harmonic-sum
    (lambda (n)
        (cond ((zero? n) 0)
              (else (+ (/ 1 n) (harmonic-sum (- n 1)))))))

(define fact
    (lambda (n)
        (if (zero? n)
            1
            (* n (fact (- n 1))))))

(define leftmost
    (lambda (l)
        (if (atom? l)
            l
            (leftmost (car l)))))

(define rightmost 
    (lambda (l)
        (if (null? (cdr l))
            (car l)
            (rightmost (cdr l)))))

(define flatten
    (lambda (ls)
        (cond ((null? ls) '())
              ((pair? (car ls))
               (append (flatten (car ls)) (flatten (cdr ls))))
              (else (cons (car ls) (flatten (cdr ls)))))))
