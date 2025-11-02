
(define escape-test
  (call/cc
    (lambda (exit)
      (for-each
        (lambda (x)
          (if (= x 3) (exit x)))
        '(1 2 3 4 5))
      0)))