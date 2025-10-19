# R3RS Scheme Interpreter

This project is an attempt to recreate **TI Scheme** in 1980.  
It is based on a small C Lisp interpreter and aims to provide a CPS continuation.

## Installation

Compile and install using:

```bash
sudo make install
```

This will create an executable named lisp in /usr/local/bin.
Usage

Start the interpreter by running:
```
scm3
```
Exit the interpreter by typing:
```
(quit)
```
![](start.png)

## Functions


## Example

```
Scheme R3RS
> (define (foo x) x)
T
> (foo 3)
3
> 

```


## Notes

This is not a full R3RS implementation, but a simplified version that captures the feel of the original.

Core functions like QUOTE, ATOM?, EQ?, CONS, CAR, CDR, and COND are implemented.

see DOCUMENT.md