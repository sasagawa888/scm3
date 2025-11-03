;; Test Framework
;; ==============================
(define (test expr expected)
  (let ((result (eval-cps expr)))
    (if (equal? result expected)
        (begin
          (display "OK: ") (display result) (newline))
        (begin
          (display expr)
          (display "ERROR: expected ") (display expected)
          (display ", but got ") (display result) (newline)))))


;; normal
(test '(catch-error '(+ 1 2)) 3)
(test '(catch-error '(* 3 4)) 12)
(test '(catch-error '(- 10 7)) 3)

;; division zero
(test '(catch-error '(/ 1 0)) 19)
(test '(catch-error '(quotient 1 0)) 19)
(test '(catch-error '(modulo 1 0)) 19)
(test '(catch-error '(remainder 1 0)) 19)

;; type error not number
(test '(catch-error '(+ 1 'a)) 4)
(test '(catch-error '(- 'a 2)) 4)
(test '(catch-error '(* 1 'b)) 4)
(test '(catch-error '(/ 'x 1)) 4)

;; undefined
(test '(catch-error '(foobar 1 2)) 18)
(test '(catch-error '(undefined-func 5)) 18)
