# Goal
This project recreates R3RS Scheme.The goal is to describe continuations using a custom CPS.

## Startup
Start the system from the terminal by running:

```
scm3

```

## REPL
After startup, the system enters the REPL.
Enter S-expressions at the prompt.
To exit, use (exit).


## Builtin Functions

```
+
-
*
/
=
>=
<=
>
<
append
append!
apply
apply-cps
and
asin
assoc
atan
begin
bind
caar
cadr
call-with-current-continuation
call-with-input-file
call-with-output-file
call/cc
car
case
cadddr
caddr
cdar
cdr
ceiling
cond
cons
delay
display
do
eq?
eqv?
equal?
expt
eval
eval-cps
even?
exp
floor
force
for-each
gbc
gcd
get
hdmp
if
inexact
lambda
length
let
let*
letrec
list
list-ref
list-tail
load
lcm
make-sting
map
memq
member
memv
modulo
negative?
newline
not
null?
number?
odd?
or
pop
positive?
procedure?
push
putprop
quote
quotient
rational?
remainder
reverse
round
set
set-car!
set-cdr!
set-clos
set!
sin
sqrt
step
symbol?
symbol->string
string-copy
string-fill!
substring
tan
truncate
unbind
vector
vector-length
vector-ref
write

```

## Sample Codes

```
; THIS IS COMMENT
(define (foo X) (+ x x))

```

## rlwrap
"scm3 is intended to have a very simple structure. Therefore, the REPL does not have any editing features. Using rlwrap makes input easier. Please run it from the terminal like this: rlwrap scm3."

```
sudo apt install rlwrap
```

## Edwin
edwin launches an editor. Please set your preferred editor in the EDITOR environment variable. If none is specified, it will call Edwin, which comes with Easy-ISLisp.

Edwin is an editor modified from Edlis for Scheme. To install it, in edwin folder, enter sudo make install. Edwin requires ncurses library. Please install ncurses.

```
sudo apt install libncurses-dev
```

## Garbage Collection
Using (gbc #t) enables output during garbage collection.
To disable it, use (gbc #f).
The system implements the classical mark-and-sweep algorithm.
You can force garbage collection by using (gbc 1).


## Immediate values:
I made integers immediate values to save cells and improve speed. By setting the second-highest bit of the integer, they are treated as positive integers. Negative numbers are outside the cell area, so they remain immediate values as they are.

## Debug
- (step #t)  stepper on.
- (step #f) stepper off.
- q   quit.

```
Scheme R3RS ver 1.20
> (define (foo x) (+ x x))
foo
> (foo 3)
6
> (step #t)
nil---cont---
(apply-cps (quote step) (pop 1)) in env[nil],stack[nil] >> #t
> (foo 3)
(3 (bind (quote x)) x (push) x (push) (apply-cps (quote +) (pop 2)) (unbind 1) (free-clos))---cont---
(set-clos foo) in env[nil],stack[(<expr> nil)] >> ((bind (quote x)) x (push) x (push) (apply-cps (quote +) (pop 2)) (unbind 1) (free-clos))---cont---
3 in env[nil],stack[(<expr> nil)] >> 
(x (push) x (push) (apply-cps (quote +) (pop 2)) (unbind 1) (free-clos))---cont---
(bind (quote x)) in env[((x . 3))],stack[(<expr> nil)] >> 
((push) x (push) (apply-cps (quote +) (pop 2)) (unbind 1) (free-clos))---cont---
x in env[((x . 3))],stack[(<expr> nil)] >> 
(x (push) (apply-cps (quote +) (pop 2)) (unbind 1) (free-clos))---cont---
(push) in env[((x . 3))],stack[(3 <expr> nil)] >> 
((push) (apply-cps (quote +) (pop 2)) (unbind 1) (free-clos))---cont---
x in env[((x . 3))],stack[(3 <expr> nil)] >> 
((apply-cps (quote +) (pop 2)) (unbind 1) (free-clos))---cont---
(push) in env[((x . 3))],stack[(3 3 <expr> nil)] >> 
((unbind 1) (free-clos))---cont---
(apply-cps (quote +) (pop 2)) in env[((x . 3))],stack[(<expr> nil)] >> 
((free-clos))---cont---
(unbind 1) in env[nil],stack[(<expr> nil)] >> 
nil---cont---
(free-clos) in env[nil],stack[nil] >> 
6
> 

```

- hdmp(N) display heap from N to N+10

## Idea for Continuation Implementation

I am inspired by Prolog’s SLD. In Prolog, SLD is a method for proving Horn clauses.
It transforms a predicate into its body.

```
P :- P1, P2, P3.
Q :- Q1, Q2, Q3.

P, Q  -> P1, P2, P3, Q  -> Q1, Q2, Q3.
```
The proof is completed when all predicates have been fully expanded.
I considered applying a similar idea to continuations in Scheme.

```
(foo x)(bar x)
```

This is expanded until it reaches primitives. When all these computations are finished, the computation is complete.

(if Test True Fail) is expanded each time it is evaluated.

## expanded function for CPS

(push)  push ACC data

(pop N)   pop N data and generate reversed list.

(bind Arg) bind Arg as ACC in ep(envinronment)

(unbind N) unbind N Arg in ep(envinronment)

(apply-cps F Args) apply function F with argument Args as CPS

(exec-cont C Arg) execute contifuation C with argument an Arg

(set-clos Sym)  Set the closure. Specifically, save the current local variable pointer onto the stack, mark the closure as being in the process of expansion, and then save the closure itself onto the stack as well.

(free-clos) Release the closure after its use has finished. Specifically, restore the local variables that were saved on the stack, then remove the saved closure from the stack and clear the mark indicating that it was in the process of expansion.

## example of CPS

- function
```
(define (foo x) (+ x 1))

(transfer '(foo 10))
((set-clos foo) 10 (bind (quote x)) x (push) 1 (push) (apply-cps + (pop 2)) (unbind 1) (free-clos))

```

- procedure
```
(transfer '(/ 2 3))
(2 (push) 3 (push) (apply-cps / (pop 2)))
> 
```
- syntax
```
(transfer '(if a b c))
((quote a) (push) (quote b) (push) (quote c) (push) (apply-cps if (pop 3)))
> 
```

- call/cc
```
transfer '(call/cc (lambda (c) (c 0))))
((quote (c)) (push) (quote (c 0)) (push) (apply-cps lambda (pop 2)) (push) (apply-cps call/cc (pop 1)))
> 
```