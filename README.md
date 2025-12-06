**“This software is a technology created for peace and freedom.  
It does not wish to serve, in any way, as a tool for oppression, suppression, human rights violations, or dictatorship.”**


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
(exit)
```
![](start.png)


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
This implementation is not fully R3RS-compliant. It does not include bignums or complex numbers. It focuses specifically on Scheme's unique continuations and closures.

see DOCUMENT.md

## A Note on Its Smallness

This system is intentionally kept small. It is not meant for practical use; rather, it is designed as a learning tool and for educational purposes.

Because the implementation is deliberately minimal, it is easy to understand as a whole. Once you have grasped its structure, you can move on to a more practical system—or simply appreciate this little interpreter for what it is.

It will probably never serve in a real application, but if browsing through its compact codebase brings you a moment of comfort or relaxation, the author could ask for nothing more.
