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
To exit, use (quit).


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

