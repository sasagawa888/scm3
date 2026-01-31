
(define foo
  (call/cc
    (lambda (ex)
      (for-each
        (lambda (x)
          (if (= x 3) (ex x)))
        '(1 2 3 4 5))
      0)))