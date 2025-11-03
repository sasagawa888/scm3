#include <string.h>
#include <stdbool.h>
#include "term.h"

#define NELEM(X) (sizeof(X) / sizeof((X)[0]))

// special form token. use in Edlis indentation
static const char *special[] = {
    "define", "let", "let*", "letrec", "case",
    "do", "begin", "lambda",
};

// syntax token
static const char *syntax[] = {
    "lambda", "letrec", "let", "let*", "set!", "define",
    "and", "or", "if", "cond", "do", "begin", "case",
    "trace", "untrace",
};

// builtin token
static const char *builtin[] = {
    "+",
    "-",
    "*",
    "/",
    "=",
    ">",
    ">=",
    "<",
    "<=",
    "append",
    "append!",
    "apply",
    "asin",
    "assoc",
    "atan",
    "atom?",
    "caar",
    "cadr",
    "call-with-current-continuation",
    "call-with-input-file",
    "call-with-ouput-file",
    "call/cc",
    "car",
    "cadddr",
    "caddr",
    "cdr",
    "ceiling",
    "cons",
    "delay",
    "display",
    "eq?",
    "eqv?",
    "equal?",
    "expt",
    "eval",
    "even?",
    "exp",
    "floor",
    "force",
    "for-each",
    "gbc",
    "gcd",
    "hdmp",
    "if",
    "inexact",
    "lambda",
    "length",
    "list",
    "list-ref",
    "list-tail",
    "load",
    "lcm",
    "make-string",
    "map",
    "memq",
    "member",
    "memv",
    "modulo",
    "negative?",
    "newline",
    "not",
    "null?",
    "number?",
    "odd?",
    "positive?",
    "procedure?",
    "quote",
    "quotient",
    "rational?",
    "remainder",
    "reverse",
    "set",
    "set-car!",
    "set-cdr!",
    "sin",
    "sqrt",
    "step",
    "string-copy",
    "string-fill",
    "substring",
    "symbol?",
    "symbol->string",
    "tan",
    "truncate",
    "vector",
    "vector-length",
    "vector-ref",
    "write",
    "zero?",
    "input-port?",
    "output-port?",
    "current-input-port",
    "current-output-port",
    "open-input-file",
    "open-output-file",
    "close-input-port",
    "close-output-port",
};

// extended function
static const char *extended[] = {

};

static bool in_syntax_table(const char *str)
{
    int i;

    for (i = 0; i < (int) NELEM(syntax); i++) {
	if (strcmp(syntax[i], str) == 0) {
	    return true;
	}
    }
    return false;
}

static bool in_builtin_table(const char *str)
{
    int i;

    for (i = 0; i < (int) NELEM(builtin); i++) {
	if (strcmp(builtin[i], str) == 0) {
	    return true;
	}
    }
    return false;
}

static bool in_extended_table(const char *str)
{
    int i;

    for (i = 0; i < (int) NELEM(extended); i++) {
	if (strcmp(extended[i], str) == 0) {
	    return true;
	}
    }
    return false;
}

bool in_special_table(const char *str)
{
    int i;

    for (i = 0; i < (int) NELEM(special); i++) {
	if (strcmp(special[i], str) == 0) {
	    return true;
	}
    }
    return false;
}

enum HighlightToken maybe_match(const char *str)
{
    if (in_syntax_table(str)) {
	return HIGHLIGHT_SYNTAX;
    }
    if (in_builtin_table(str)) {
	return HIGHLIGHT_BUILTIN;
    }
    if (in_extended_table(str)) {
	return HIGHLIGHT_EXTENDED;
    }
    return HIGHLIGHT_NONE;
}

void
gather_fuzzy_matches(const char *str, const char *candidates[],
		     int *candidate_pt)
{
    int i;

    for (i = 0; i < (int) NELEM(syntax); i++) {
	if (strstr(syntax[i], str) != NULL && syntax[i][0] == str[0]) {
	    candidates[*candidate_pt] = syntax[i];
	    *candidate_pt = (*candidate_pt) + 1;
	}
    }
    for (i = 0; i < (int) NELEM(builtin); i++) {
	if (strstr(builtin[i], str) != NULL && builtin[i][0] == str[0]) {
	    candidates[*candidate_pt] = builtin[i];
	    *candidate_pt = (*candidate_pt) + 1;
	}
    }
    for (i = 0; i < (int) NELEM(extended); i++) {
	if (strstr(extended[i], str) != NULL && extended[i][0] == str[0]) {
	    candidates[*candidate_pt] = extended[i];
	    *candidate_pt = (*candidate_pt) + 1;
	}
    }
}
