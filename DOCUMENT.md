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

## Sample Codes

```
; THIS IS COMMENT
(define (foo X) (+ x x))

```

## Edwin
edwin launches an editor. Please set your preferred editor in the EDITOR environment variable. If none is specified, it will call Edlis, which comes with Easy-ISLisp.

## Garbage Collection
Using (gbc t) enables output during garbage collection.
To disable it, use (gbc nil).
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


## expanded function for CPS

(push)  push ACC data

(pop N)   pop N data and generate reversed list.

(bind Arg) bind Arg as ACC in ep(envinronment)

(unbind N) unbind N Arg in ep(envinronment)

(apply-cps F Args) apply function F with argument Args as CPS

(exec-cont C Arg) execute contifuation C with argument an Arg

(set-clos Sym)  Set the closure. Specifically, save the current local variable pointer onto the stack, mark the closure as being in the process of expansion, and then save the closure itself onto the stack as well.

(free-clos) Release the closure after its use has finished. Specifically, restore the local variables that were saved on the stack, then remove the saved closure from the stack and clear the mark indicating that it was in the process of expansion.
