# Idea Memo

## call/cc 

cp continuation pointer

- (+ 1 2 (call/cc ...)) 
(push (eval 1st)) (push (eval 2nd)) (push (eval 3rd)) (funcall '+ (pop3))

- (lambda (x y) (+ x (call/cc)))
(set! x (eval 1st)) (set! y (eval 2nd)) (funcall (lambda (x y) (+ x y)) (pop2))

- progn (foo) (call/cc) (bar) (boo)
(foo) (call/cc) (bar) (boo) 

- (if (foo) (bar) (boo))
(push (eval (foo))) (push (eval (bar))) (push (eval boo)) (fsubrcall (pops3))

- (if (foo) (bar) (call/cc))
(push (eval (foo))) (push (eval (bar))) (push (eval boo)) (fsubrcall (pops3))

stack[foo-val,bar-val,lambda(cont)]

cont object lambda(cont)=bind(ep)=bind[stack];
eval cont ep=bind(ep),stack=bind[stack] call(lambda)


