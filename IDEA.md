# Idea Memo

## call/cc 

cp continuation pointer
acc <= response (eval)
(push) push response(acc)

- (+ 1 2 (call/cc ...)) 
(1st) (push) (2nd) (push) (3rd) (push) (apply + (pop 3))

- (lambda (x y) (+ x (call/cc)))
(1st) (bind x) (2nd) (bind y) (apply (lambda (x y) (+ x y)) (list x y) (unbind 3))

- progn (foo) (call/cc) (bar) (boo)
(foo) (call/cc) (bar) (boo) 

- (foo (foo) (bar) (boo))
(foo) (push) (bar) (push) (boo) (push) (apply foo (pops 3))

- (if (foo) (bar) (boo))
(apply if (foo) (bar) (boo)) 
Special forms capture call/cc during the evaluation of each syntactic construct.

- (define (foo x) x) - (foo 3)
3 (bind x) x (unbind 1) (push)




stack (lambda(cont) bar-val foo-val) 

cont object lambda(cont)=bind(ep)=bind(stack);
eval cont ep=bind(ep),stack=bind(stack) call(lambda)

When (foo) is a complex composite function, the continuation pointer (cp) changes during the evaluation process as (foo1)(foo2)(foo3). Furthermore, these may transform into subrs: (subr1)(subr2)(subr3)(subr4)(foo3). … If call/cc occurs at (foo2), then …

A continuation object is similar to a lambda, but differs from a normal lambda in that it also saves the stack. It is represented as a separate object called cont.

