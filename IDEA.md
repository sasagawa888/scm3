# Idea Memo

## call/cc 

cp continuation pointer
acc <= response (eval)
(push) push response(acc)

- (+ 1 2 (call/cc ...)) 
(1st) (push) (2nd) (push) (3rd) (push) (apply + (pop 3))

- (lambda (x y) (+ x (call/cc)))
(1st) (push) (set! x (pop)) (2nd) (push) (set! y (pop)) (apply (lambda (x y) (+ x y)) (list x y))

- progn (foo) (call/cc) (bar) (boo)
(foo) (call/cc) (bar) (boo) 

- (if (foo) (bar) (boo))
(foo) (push) (bar) (push) (boo) (push) (apply if (pops 3))

- (if (foo) (bar) (call/cc))
(foo) (push) (bar) (push) (boo) (push) (apply if (pops 3))

stack (lambda(cont) bar-val foo-val) 

cont object lambda(cont)=bind(ep)=bind(stack);
eval cont ep=bind(ep),stack=bind(stack) call(lambda)

When (foo) is a complex composite function, the continuation pointer (cp) changes during the evaluation process as (foo1)(foo2)(foo3). Furthermore, these may transform into subrs: (subr1)(subr2)(subr3)(subr4)(foo3). … If call/cc occurs at (foo2), then …
