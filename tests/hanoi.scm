

(define (hanoi n)
    (hanoi-aux n 'a 'c 'b))

(define (hanoi-aux n from to free)
    (if (= n 1)
        (move-disk 1 from to)
        (begin 
            (hanoi-aux (- n 1) from free to)
            (move-disk n from to)
            (hanoi-aux (- n 1) free to from))))


(define (move-disk disk from to)
    (display "Move disk ")
    (display disk)
    (display " from ")
    (display from)
    (display " to ")
    (display to)
    (display ".")
    (newline))