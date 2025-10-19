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
> (define (foo x) (+ x x))
foo
> (foo 3)
6
> (step #t)
(apply (quote step) (pop 1)) in [] >> t
> (foo 3)
(3 (bind (quote x)) x (push) x (push) (apply (quote +) (pop 2)) (unbind 1))3 in [] >> (bind (quote x)) in [(x . 3)] >> 
x in [(x . 3)] >> 
x in [(x . 3)] >> 
(push) in [(x . 3)] >> 
x in [(x . 3)] >> 
x in [(x . 3)] >> 
(push) in [(x . 3)] >> 
(apply (quote +) (pop 2)) in [(x . 3)] >> 
(unbind 1) in [] >> 
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
