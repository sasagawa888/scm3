/*
Scheme R3RS  
written by kenichi sasagawa

*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <setjmp.h>
#include <float.h>
#include <math.h>
#include <signal.h>
#include "scm3.h"

double version = 0.05;
cell heap[HEAPSIZE];
int stack[STACKSIZE];
int argstk[STACKSIZE];
token stok = { GO, OTHER };

jmp_buf buf;
int cell_hash_table[HASHTBSIZE];

FILE *input_stream;
int gbc_flag = 0;
int return_flag = 0;
int step_flag = 0;
int gennum = 1;
int oblist_len;
int ctrl_c_flag;

void signal_handler_c(int signo)
{
    ctrl_c_flag = 1;
}

int main(int argc, char *argv[])
{

    printf("Scheme R3RS ver %.2f\n", version);
    input_stream = stdin;
    initcell();
    initsubr();
    signal(SIGINT, signal_handler_c);
    
    input_stream = stdin;
    int ret = setjmp(buf);

  repl:
    if (ret == 0)
	while (1) {
	    printf("> ");
	    fflush(stdout);
	    fflush(stdin);
        cp = NIL;
        cpssp = NIL;
	    print(eval_cps(read()));
	    printf("\n");
	    fflush(stdout);
    } else if (ret == 1) {
	ret = 0;
	goto repl;
    } else
	return 0;
}

//-----------------------------
void initcell(void)
{
    int addr, x;

    for (addr = 0; addr < HEAPSIZE; addr++) {
	heap[addr].flag = FRE;
	heap[addr].cdr = addr + 1;
    }
    hp = 0;
    fc = HEAPSIZE;

    for (x = 0; x < HASHTBSIZE; x++)
	cell_hash_table[x] = NIL;

    // Address 0 is NIL; set up the environment registers.
    // This is the initial environment."
    ep = makesym("nil");
    assocsym(makesym("nil"), NIL);
    assocsym(makesym("t"), makesym("t"));

    sp = 0;
    ap = 0;
}

int freshcell(void)
{
    int res;

    res = hp;
    hp = heap[hp].cdr;
    SET_CDR(res, 0);
    fc--;
    return (res);
}

// Done via deep-bind. If the symbol is not found, register it.
// If found, store the value there.
void bindsym(int sym, int val)
{
    int addr;

    addr = assoc(sym, ep);
    if (addr == NO)
	assocsym(sym, val);
    else
	SET_CDR(addr, val);
}

// Variable binding
// For local variables, bindings are stacked on top of previous ones.
// Therefore, even if the same variable name exists, it is not overwritten.
void assocsym(int sym, int val)
{
    ep = cons(cons(sym, val), ep);
}



// The environment is represented as an association list like this:
// env = ((sym1 . val1) (sym2 . val2) ... (t . t) (nil . 0))
// Use `assoc` to find the value corresponding to a symbol.
// If not found, it returns -1.
int findsym(int sym)
{
    int addr;

    addr = assoc(sym, ep);

    if (addr == NO)
	return (NO);
    else
	return (cdr(addr));
}

// Used to ensure the uniqueness of symbols
int getsym(char *name, int index)
{
    int addr;

    addr = cell_hash_table[index];

    while (addr != NIL) {
	if (strcmp(name, GET_NAME(car(addr))) == 0)
	    return (car(addr));
	else
	    addr = cdr(addr);
    }
    return (0);
}

int addsym(char *name, int index)
{
    int addr, res;

    addr = cell_hash_table[index];
    addr = cons(res = makesym1(name), addr);
    cell_hash_table[index] = addr;
    return (res);
}

int makesym1(char *name)
{
    int addr;

    addr = freshcell();
    SET_TAG(addr, SYM);
    SET_NAME(addr, name);
    return (addr);
}


int hash(char *name)
{
    int res;

    res = 0;
    while (*name != NUL) {
	res = res + (int) *name;
	name++;
    }
    return (res % HASHTBSIZE);
}

//-------for debug------------------    
void cellprint(int addr)
{
    switch (GET_FLAG(addr)) {
    case FRE:
	printf("FRE ");
	break;
    case USE:
	printf("USE ");
	break;
    }
    switch (GET_TAG(addr)) {
    case EMP:
	printf("EMP    ");
	break;
    case NUM:
	printf("NUM    ");
	break;
    case FLTN:
	printf("FLT    ");
	break;
    case SYM:
	printf("SYM    ");
	break;
    case STR:
	printf("STR    ");
	break;
    case LIS:
	printf("LIS    ");
	break;
    case SUBR:
	printf("SUBR   ");
	break;
    case FSUBR:
	printf("FSUBR  ");
	break;
    case EXPR:
	printf("EXPR   ");
	break;
    case FEXPR:
	printf("EXPR   ");
	break;
    case MACRO:
	printf("MACRO  ");
	break;
    }
    printf("%07d ", GET_CAR(addr));
    printf("%07d ", GET_CDR(addr));
    printf("%07d ", GET_BIND(addr));
    printf("%s \n", GET_NAME(addr));
}

void heapdump(int start, int end)
{
    int i;

    printf("addr    F   TAG   CAR     CDR    BIND   NAME\n");
    for (i = start; i <= end; i++) {
	printf("%07d ", i);
	cellprint(i);
    }
}

//---------GC-----------
void gbc(void)
{
    int addr;

    if (gbc_flag) {
	printf("enter GBC free=%d\n", fc);
	fflush(stdout);
    }
    gbcmark();
    gbcsweep();
    fc = 0;
    for (addr = 0; addr < HEAPSIZE; addr++) {
	if (IS_EMPTY(addr))
	    fc++;
    }
    if (gbc_flag) {
	printf("exit GBC free=%d\n", fc);
	fflush(stdout);
    }
}


void markoblist(void)
{
    int addr;

    addr = ep;
    while (!(nullp(addr))) {
	MARK_CELL(addr);
	addr = cdr(addr);
    }
    MARK_CELL(0);
}

void markcell(int addr)
{
    if (integerp(addr))
	return;
    if (USED_CELL(addr))
	return;

    MARK_CELL(addr);

    if (car(addr) != 0) {
	markcell(car(addr));
    }
    if (cdr(addr) != 0) {
	markcell(cdr(addr));
    }

    if ((GET_BIND(addr) != 0) && IS_EXPR(addr))
	markcell(GET_BIND(addr));

    if ((GET_BIND(addr) != 0) && IS_MACRO(addr))
	markcell(GET_BIND(addr));

    if ((GET_BIND(addr) != 0) && IS_FEXPR(addr))
	markcell(GET_BIND(addr));


}

void gbcmark(void)
{
    int addr, i;

    //Mark oblist
    markoblist();
    // Mark the cells linked from the oblist.
    addr = ep;
    while (!(nullp(addr))) {
	markcell(car(addr));
	addr = cdr(addr);
    }
    // Mark the cells that are bound from the argstack.
    for (i = 0; i < ap; i++)
	markcell(argstk[i]);

    // Mark the cells linked from the symbol hash table.
    for (i = 0; i < HASHTBSIZE; i++)
	markcell(cell_hash_table[i]);
}

void gbcsweep(void)
{
    int addr;

    addr = 0;
    while (addr < HEAPSIZE) {
	if (USED_CELL(addr))
	    NOMARK_CELL(addr);
	else {
	    clrcell(addr);
	    SET_CDR(addr, hp);
	    hp = addr;
	}
	addr++;
    }
}

void clrcell(int addr)
{
    SET_TAG(addr, EMP);
    free(heap[addr].name);
    heap[addr].name = NULL;
    SET_CAR(addr, 0);
    SET_CDR(addr, 0);
    SET_BIND(addr, 0);
    SET_TR(addr, 0);
}

// If the number of free cells falls below a certain threshold, trigger GBC.
void checkgbc(void)
{
    if (ctrl_c_flag){
        ctrl_c_flag = 0;
        longjmp(buf, 1);
    }
    if (fc < FREESIZE)
	gbc();
}

//-------------data type-----------------

int atomp(int addr)
{
    if (integerp(addr))
	return (1);
    if (IS_FLT(addr) || IS_STR(addr) || IS_SYMBOL(addr))
	return (1);
    else
	return (0);
}


int integerp(int addr)
{
    if (addr >= INT_FLAG)
	return (1);
    else if (addr < 0)
	return (1);
    else
	return (0);
}

int floatp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_FLT(addr))
	return (1);
    else
	return (0);
}

int stringp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_STR(addr))
	return (1);
    else
	return (0);
}

int numberp(int addr)
{
    if (integerp(addr))
	return (1);
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_FLT(addr))
	return (1);
    else
	return (0);
}

int fixp(int addr)
{
    if (integerp(addr))
	return (1);
    else
	return (0);
}


int symbolp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_SYMBOL(addr))
	return (1);
    else
	return (0);
}

int listp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_LIST(addr) || IS_NIL(addr))
	return (1);
    else
	return (0);
}

int nullp(int addr)
{
    if (IS_NIL(addr))
	return (1);
    else
	return (0);
}


int eqp(int addr1, int addr2)
{
    if (addr1 == addr2)
	return (1);
    else if (floatp(addr1) && floatp(addr2)
	     && GET_FLT(addr1) - GET_FLT(addr2) < DBL_MIN)
	return (1);
    else if ((symbolp(addr1)) && (symbolp(addr2))
	     && (SAME_NAME(addr1, addr2)))
	return (1);
    else
	return (0);
}

int equalp(int x, int y)
{
    if (atomp(x) && atomp(y))
	return (eqp(x, y));
    else if (eqp(car(x), car(y)) && equalp(cdr(x), cdr(y)))
	return (1);

    return (NIL);
}


int subrp(int addr)
{
	return (IS_SUBR(GET_BIND(addr)));
}

int fsubrp(int addr)
{
	return (IS_FSUBR(GET_BIND(addr)));
}

int functionp(int addr)
{
	return (IS_EXPR(GET_BIND(addr)));
}

int experp(int addr){

    if(symbolp(addr))
        return(IS_EXPR(findsym(addr)));
    else 
        return(0);
}

int fexprp(int addr)
{
	return (IS_FEXPR(GET_BIND(addr)));
}


int lambdap(int addr)
{
    if(listp(addr) && car(addr) == makesym("lambda"))
        return(1);
    else 
        return (0);
}

int macrop(int addr)
{
	return (IS_MACRO(GET_BIND(addr)));
}

//--------------list---------------------

int car(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (NIL);
    return (GET_CAR(addr));
}

int caar(int addr)
{
    return (car(car(addr)));
}

int cdar(int addr)
{
    return (cdr(car(addr)));
}

int cdr(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (NIL);
    return (GET_CDR(addr));
}

int cadr(int addr)
{
    return (car(cdr(addr)));
}

int cddr(int addr)
{
    return (cdr(cdr(addr)));
}

int caddr(int addr)
{
    return (car(cdr(cdr(addr))));
}

int cadar(int addr)
{
    return (car(cdr(car(addr))));
}

int cadddr(int addr)
{
    return (car(cdr(cdr(cdr(addr)))));
}



int cons(int car, int cdr)
{
    int addr;

    addr = freshcell();
    SET_TAG(addr, LIS);
    SET_CAR(addr, car);
    SET_CDR(addr, cdr);
    return (addr);
}

int assoc(int sym, int lis)
{
    if (nullp(lis))
	return (NO);
    else if (eqp(sym, caar(lis)))
	return (car(lis));
    else
	return (assoc(sym, cdr(lis)));
}

int member(int x, int lis)
{
    if (nullp(lis))
	return (NIL);
    else if (equalp(x, car(lis)))
	return (T);
    else
	return (member(x, cdr(lis)));
}

int reverse(int addr)
{
    int res;
    res = NIL;
    while(!nullp(addr)){
        res = cons(car(addr),res);
        addr = cdr(addr);
    }
    return(res);
}

int length(int addr)
{
    int len = 0;

    while (!(nullp(addr))) {
	len++;
	addr = cdr(addr);
    }
    return (len);
}

int list(int arglist)
{
    if (nullp(arglist))
	return (NIL);
    else
	return (cons(car(arglist), list(cdr(arglist))));
}

int append(int x, int y)
{
    if (nullp(x))
	return (y);
    else
	return (cons(car(x), append(cdr(x), y)));
}

int last(int x)
{
    if (nullp(cdr(x)))
	return (x);
    else
	return (last(cdr(x)));
}

int nconc(int x, int y)
{
    if (nullp(x))
	return (y);

    SET_CDR(last(x), y);
    return (x);
}

int mapcar(int lis, int fun)
{
    if (nullp(lis))
	return (NIL);
    else
	return (cons(apply(fun, list1(car(lis))), mapcar(cdr(lis), fun)));
}

int mapcon(int lis, int fun)
{
    if (nullp(lis))
	return (NIL);
    else
	return (nconc
		(list1(apply(fun, list1(car(lis)))),
		 mapcon(cdr(lis), fun)));
}

int map(int lis, int fun)
{
    while (!nullp(lis)) {
	apply(fun, list1(car(lis)));
	lis = cdr(lis);
    }
    return (NIL);
}

//----------------------------------------
int get_int(int addr)
{
    if (addr >= 0)
	return (INT_MASK & addr);
    else
	return (addr);
}

int makeint(int num)
{
    if (num >= 0)
	return (INT_FLAG | num);
    else
	return (num);
}


int makeflt(double floatn)
{
    int addr;

    addr = freshcell();
    SET_TAG(addr, FLTN);
    SET_FLT(addr, floatn);
    return (addr);
}

int makestr(char *name)
{
    int addr;

    addr = freshcell();
    SET_TAG(addr, STR);
    SET_NAME(addr, name);
    return (addr);
}


int makesym(char *name)
{
    int index, res;

    index = hash(name);
    if ((res = getsym(name, index)) != 0)
	return (res);
    else
	return (addsym(name, index));
}


int makefunc(int addr)
{
    int val;

    val = freshcell();
    SET_TAG(val, EXPR);
    SET_BIND(val, addr);
    SET_CDR(val, 0);
    return (val);
}

//-------for CPS--------------------
void cps_push(int addr)
{
    cpssp = cons(addr,cpssp);
}

int cps_pop(int n)
{
    int res;
    res = NIL;
    while(n>0){
        res = cons(car(cpssp),res);
        cpssp = cdr(cpssp);
        n--;
    }
    return(res);
}

void cps_bind(int addr)
{
    assocsym(addr,acc);
}

void cps_unbind(int n)
{
    int new;
    new = ep;
    while(n > 0){
        new = cdr(new);
        n--;
    }
    ep = new;
}

// Stack. Used to save the environment pointer (EP).
void push(int pt)
{
    stack[sp++] = pt;
}

int pop(void)
{
    return (stack[--sp]);
}

// Push/pop for the arglist stack
void argpush(int addr)
{
    argstk[ap++] = addr;
}

void argpop(void)
{
    --ap;
}

//-------read()--------

void gettoken(void)
{
    char c;
    int pos;

    if (stok.flag == BACK) {
	stok.flag = GO;
	return;
    }

    if (stok.ch == ')') {
	stok.type = RPAREN;
	stok.ch = NUL;
	return;
    }

    if (stok.ch == '(') {
	stok.type = LPAREN;
	stok.ch = NUL;
	return;
    }

    skip:
    c = fgetc(input_stream);
    while ((c == SPACE) || (c == EOL) || (c == TAB))
	c = fgetc(input_stream);

    if(c == ';'){
        while (c != EOL){
	        c = fgetc(input_stream);
        }
        goto skip;
    }

    switch (c) {
    case '(':
	stok.type = LPAREN;
	break;
    case ')':
	stok.type = RPAREN;
	break;
    case '\'':
	stok.type = QUOTE;
	break;
    case '.':
	stok.type = DOT;
	break;
    case '"':
    pos = 0;
	while (((c = fgetc(input_stream)) != EOL) && (pos < BUFSIZE) &&
		   (c != '"'))
	stok.buf[pos++] = c;

	stok.buf[pos] = NUL;
    stok.type = STRING;
    return;
    case '|':
    pos = 0;
    stok.buf[pos++] = c;
	while (((c = fgetc(input_stream)) != EOL) && (pos < BUFSIZE) &&
		   (c != '|'))
	stok.buf[pos++] = c;
    
    stok.buf[pos++] = c;
	stok.buf[pos] = NUL;
    stok.type = SYMBOL;
    return;
    case '`':
	stok.type = BACKQUOTE;
	break;
    case ',':
	stok.type = COMMA;
	break;
    case '@':
	stok.type = ATMARK;
	break;
    case EOF:
	stok.type = FILEEND;
	return;
    default:{
	    pos = 0;
	    stok.buf[pos++] = c;
	    while (((c = fgetc(input_stream)) != EOL) && (pos < BUFSIZE) &&
		   (c != SPACE) && (c != '(') && (c != ')') &&
		   (c != '`') && (c != ',') && (c != '@'))
		stok.buf[pos++] = c;

	    stok.buf[pos] = NUL;
	    stok.ch = c;
	    if (inttoken(stok.buf)) {
		stok.type = INTEGER;
		break;
	    }
	    if (flttoken(stok.buf)) {
		stok.type = FLOAT;
		break;
	    }
	    if (symboltoken(stok.buf)) {
		stok.type = SYMBOL;
		break;
	    }
	    stok.type = OTHER;
	}
    }
}

int inttoken(char buf[])
{
    int i;
    char c;

    if (((buf[0] == '+') || (buf[0] == '-'))) {
	if (buf[1] == NUL)
	    return (0);		// case {+,-} => symbol
	i = 1;
	while ((c = buf[i]) != NUL)
	    if (isdigit(c))
		i++;		// case {+123..., -123...}
	    else
		return (0);
    } else {
	i = 0;			// {1234...}
	while ((c = buf[i]) != NUL)
	    if (isdigit(c))
		i++;
	    else
		return (0);
    }
    return (1);
}

int flttoken(char buf[])
{
    int i;
    char c;

    if (((buf[0] == '+') || (buf[0] == '-'))) {
	if (buf[1] == NUL)
	    return (0);		// case {+,-} => symbol
	i = 1;
	while ((c = buf[i]) != NUL)
	    if (isdigit(c))
		i++;		// case {+123..., -123...}
	    else
		return (0);
    } else {
	i = 0;			// {1234...}
	while ((c = buf[i]) != NUL)
	    if (isdigit(c))
		i++;
	    else if (c == '.')
		goto dot;
	    else if (c == 'e' || c == 'E')
		goto exp;
	    else
		return (0);
    }
    return (0);

  dot:
    i++;
    while ((c = buf[i]) != NUL) {
	if (isdigit(c))
	    i++;
	else if (c == 'e' || c == 'E')
	    goto exp;
	else
	    return (0);
    }
    return (1);

  exp:
    i++;
    while ((c = buf[i]) != NUL) {
	if (isdigit(c))
	    i++;
	else
	    return (0);
    }
    return (1);
}


int symboltoken(char buf[])
{
    int i;
    char c;


    i = 0;
    while ((c = buf[i]) != NUL)
	if ((isalpha(c)) || (isdigit(c)) || (issymch(c)))
	    i++;
	else
	    return (0);

    return (1);
}

int issymch(char c)
{
    switch (c) {
    case '!':
    case '?':
    case '+':
    case '-':
    case '*':
    case '/':
    case '=':
    case '<':
    case '>':
	return (1);
    default:
	return (0);
    }
}


int read(void)
{
    gettoken();
    switch (stok.type) {
    case FILEEND:
	return (EOF);
    case INTEGER:
	return (makeint(atoi(stok.buf)));
    case FLOAT:
	return (makeflt(atof(stok.buf)));
    case STRING:
    return (makestr(stok.buf));
    case SYMBOL:
	return (makesym(stok.buf));
    case QUOTE:
	return (cons(makesym("quote"), cons(read(), NIL)));
    case BACKQUOTE:
	return (cons(makesym("quasi-quote"), cons(read(), NIL)));
    case COMMA:{
	    gettoken();
	    if (stok.type == ATMARK)
		return (cons
			(makesym("unquote-splicing"), cons(read(), NIL)));
	    else {
		stok.flag = BACK;
		return (cons(makesym("unquote"), cons(read(), NIL)));
	    }
	}
    case LPAREN:
	return (readlist());
    default:
	break;
    }
    error(CANT_READ_ERR, "read", NIL);
    return (0);
}

int readlist(void)
{
    int car, cdr;

    gettoken();
    if (stok.type == RPAREN)
	return (NIL);
    else if (stok.type == DOT) {
	cdr = read();
	if (atomp(cdr))
	    gettoken();
	return (cdr);
    } else {
	stok.flag = BACK;
	car = read();
	cdr = readlist();
	return (cons(car, cdr));
    }
}

//-----print------------------
void print(int addr)
{
    if (integerp(addr)) {
	printf("%d", GET_INT(addr));
	return;
    }
    switch (GET_TAG(addr)) {
    case FLTN:
	printf("%g", GET_FLT(addr));
	if (GET_FLT(addr) - (int) GET_FLT(addr) == 0.0)
	    printf(".0");
	break;
    case STR:
	printf("\"%s\"", GET_NAME(addr));
	break;
    case SYM:
	printf("%s", GET_NAME(addr));
	break;
    case SUBR:
	printf("<subr>");
	break;
    case FSUBR:
	printf("<fsubr>");
	break;
    case EXPR:
	printf("<expr>");
	break;
    case FEXPR:
	printf("<fexpr>");
	break;
    case MACRO:
	printf("<macro>");
	break;
    case LIS:{
	    printf("(");
	    printlist(addr);
	    break;
	}
    default:
	printf("<undef>");
	break;
    }
}


void printlist(int addr)
{
    if (IS_NIL(addr))
	printf(")");
    else if ((!(listp(cdr(addr)))) && (!(nullp(cdr(addr))))) {
	print(car(addr));
	printf(" . ");
	print(cdr(addr));
	printf(")");
    } else {
	print(GET_CAR(addr));
	if (!(IS_NIL(GET_CDR(addr))))
	    printf(" ");
	printlist(GET_CDR(addr));
    }
}

void princ(int addr)
{
    char c,str[SYMSIZE];
    int pos;
    if (integerp(addr)) {
	printf("%d", GET_INT(addr));
	return;
    }
    switch (GET_TAG(addr)) {
    case FLTN:
	printf("%g", GET_FLT(addr));
	if (GET_FLT(addr) - (int) GET_FLT(addr) == 0.0)
	    printf(".0");
	break;
    case STR:
	printf("%s", GET_NAME(addr));
	break;
    case SYM:
    memset(str,0,SYMSIZE);
    pos = 0;
    strcpy(str,GET_NAME(addr));
    c = str[pos++];
    while(c != 0){
        if(c != '|')
            printf("%c",c);
        c = str[pos++];
    }
	break;
    case SUBR:
	printf("<subr>");
	break;
    case FSUBR:
	printf("<fsubr>");
	break;
    case EXPR:
	printf("<expr>");
	break;
    case FEXPR:
	printf("<fexpr>");
	break;
    case MACRO:
	printf("<macro>");
	break;
    case LIS:{
	    printf("(");
	    printlist(addr);
	    break;
	}
    default:
	printf("<undef>");
	break;
    }
}



//--------eval---------------
// transfer expr arguments
int transfer_exprargs(int args,int varlist)
{
    if(nullp(args))
        return(NIL);
    else 
        return(cons(car(args),
                   cons(list2(makesym("bind"),list2(makesym("quote"),car(varlist))),
                      transfer_exprargs(cdr(args),cdr(varlist)))));
}

int transfer_exprbody(int addr)
{
    if(nullp(addr))
        return(NIL);
    else 
        return(append(transfer(car(addr)),transfer_exprbody(cdr(addr))));
}

int transfer_subrargs(int args)
{
    if(nullp(args))
        return(NIL);
    else {
        return(cons(car(args),
                  cons(list1(makesym("push")),
                      transfer_subrargs(cdr(args)))));
    }

}

int transfer_fsubrargs(int args)
{
    if(nullp(args))
        return(NIL);
    else {
        return(cons(list2(makesym("quote"),car(args)),
                  cons(list1(makesym("push")),
                      transfer_fsubrargs(cdr(args)))));
    }
}

int transfer(int addr)
{   
    int func,varlist,args,body;
    if(numberp(addr) || stringp(addr) || symbolp(addr))
        return(list1(addr));
    else if(subrp(car(addr))){
        args = transfer_subrargs(cdr(addr));
        body = list3(makesym("apply"),list2(makesym("quote"),car(addr)),
                  list2(makesym("pop"),makeint(length(cdr(addr)))));
        return(append(args,list1(body)));
    }
    else if(fsubrp(car(addr))){
        args = transfer_fsubrargs(cdr(addr));
        body = list3(makesym("apply"),list2(makesym("quote"),car(addr)),
                  list2(makesym("pop"),makeint(length(cdr(addr)))));
        return(append(args,list1(body)));
    }
    else if(functionp(car(addr))){
        func = car(addr);
        varlist = car(GET_BIND(GET_BIND(func)));
	    body = transfer_exprbody(cdr(GET_BIND(GET_BIND(func))));
        args = transfer_exprargs(cdr(addr),varlist);
        return(append(args,append(body,
                 list1(list2(makesym("unbind"),makeint(length(cdr(addr))))))));
    }

    return(NIL);
}

// execute continuation
int execute_args(int addr)
{
    if(nullp(addr))
        return(NIL);
    else 
        return(cons(execute(car(addr)),execute_args(cdr(addr))));
}

int execute(int addr)
{
    if(numberp(addr) || stringp(addr))
        return(addr);
    else if(listp(addr)){
        if(eqp(car(addr),makesym("quote")))
            return(cadr(addr));
        else if(subrp(car(addr)) || fsubrp(car(addr))){
            int func,args;
            func = GET_BIND(car(addr));
            args = execute_args(cdr(addr));
            return ((GET_SUBR(func)) (args));
        }
    }
    return(eval(addr));
}


void print_env(void){
    int i,n,env;

    n = length(ep) - oblist_len;
    env = ep;
    printf("[");
    for(i=0;i<n;i++){
        print(car(env));
        env = cdr(env);
    }
    printf("]");
}


// eval CPS
int eval_cps(int addr)
{   
    int exp,c;
    cp = append(transfer(addr),cp);
    if(step_flag)
        print(cp);
    while(!nullp(cp)){
        exp = car(cp);
        cp = cdr(cp);
        acc = execute(exp);
        if(step_flag){
        print(exp);
        printf(" in ");
        print_env();
        printf(" >> ");
        c = getc(stdin);
        if(c == 'q')
            longjmp(buf, 1);
        }
    }
    return(acc);
}


int eval(int addr)
{
    int res;

    if(step_flag){
        int c;
        print(addr);
        printf(" in ");
        print_env();
        printf(" >> ");
        c = getc(stdin);
        if(c == 'q')
            longjmp(buf, 1);
    }
    if (atomp(addr)) {
	if (numberp(addr) || stringp(addr))
	    return (addr);
	if (symbolp(addr)) {
	    res = findsym(addr);
	    if (res == NO)
		error(CANT_FIND_ERR, "eval", addr);
	    else
		return (res);
	}
    } else if (listp(addr)) {
	if (numberp(car(addr)))
	    error(ARG_SYM_ERR, "eval", addr);
	else if ((symbolp(car(addr)))
		 && (eqp(car(addr), makesym("quasi-quote"))))
	    return (eval(quasi_transfer2(cadr(addr), 0)));
    else if (lambdap(car(addr)))
        return (apply(eval(car(addr)), cdr(addr)));
	else if (subrp(car(addr)))
	    return (apply(GET_BIND(car(addr)), evlis(cdr(addr))));
	else if (fsubrp(car(addr)))
	    return (apply(GET_BIND(car(addr)), cdr(addr)));
	else if (functionp(car(addr))){
        int sym,i,n,res;
        sym = car(addr);
        if(GET_TR(sym) >= 1){
            SET_TR(sym,GET_TR(sym)+1);
            n = GET_TR(sym);
            for(i=2;i<n;i++){
                printf(" ");
            }
            printf("ENTER ");
            print(sym);
            print(evlis(cdr(addr)));
            printf("\n");
        }
	    res = apply(GET_BIND(car(addr)), evlis(cdr(addr)));
        if (GET_TR(sym) > 1){
            n = GET_TR(sym);
            for(i=2;i<n;i++){
                printf(" ");
            }
            printf("RETURN ");
            print(sym);
            printf(" ");
            print(res);
            printf("\n");
            SET_TR(sym,GET_TR(sym)-1);
        }
        return(res);
    }
    else if (experp(car(addr))){
        int func;
        func = findsym(car(addr));
        res = apply(func, evlis(cdr(addr)));
        return(res);
    }
	else if (macrop(car(addr)))
	    return (apply(GET_BIND(car(addr)), cdr(addr)));

    else if (fexprp(car(addr)))
        return (apply(GET_BIND(car(addr)), cdr(addr)));
    }
    error(CANT_FIND_ERR, "eval", addr);
    return (0);
}


int apply_cps(int func, int args)
{
    int body, res;

    res = NIL;
    switch (GET_TAG(func)) {
    case SUBR:
	return ((GET_SUBR(func)) (args));
    case FSUBR:
	return ((GET_SUBR(func)) (args));
    case EXPR:
    {
	    body = cdr(GET_BIND(func));
        cp = append(body,cp);
	    while (!(IS_NIL(cp))) {
		res = car(cp);
		cp = cdr(cp);
        res = eval_cps(res);
	    }
	    return (res);
	}
    
    default:
	error(ILLEGAL_OBJ_ERR, "apply", func);
    }
    return (0);
}


int apply(int func, int args)
{
    int varlist, body, res, macrofunc;

    res = NIL;
    switch (GET_TAG(func)) {
    case SUBR:
	return ((GET_SUBR(func)) (args));
    case FSUBR:
	return ((GET_SUBR(func)) (args));
    case EXPR:{
	    varlist = car(GET_BIND(func));
	    body = cdr(GET_BIND(func));
        cp = append(body,cp);
	    bindarg(varlist, args);
	    while (!(IS_NIL(cp))) {
		res = car(cp);
		cp = cdr(cp);
        res = eval(res);
	    }
	    unbind();
	    return (res);
	}
    case MACRO:{
	    macrofunc = GET_BIND(func);
	    varlist = car(GET_BIND(macrofunc));
	    body = cdr(GET_BIND(macrofunc));
	    bindarg(varlist, list1(cons(makesym("_"),args)));
	    while (!(IS_NIL(body))) {
		res = eval(car(body));
		body = cdr(body);
	    }
	    unbind();
	    res = eval(res);
	    return (res);
	}
    case FEXPR:{
	    varlist = car(GET_BIND(func));
	    body = cdr(GET_BIND(func));
        cp = cons(body,cp);
	    bindarg(varlist, list1(args));
	    while (!(IS_NIL(cp))) {
		res = eval(car(cp));
		cp = cdr(cp);
	    }
	    unbind();
	    return (res);
	}
    default:
	error(ILLEGAL_OBJ_ERR, "apply", func);
    }
    return (0);
}

void bindarg(int varlist, int arglist)
{
    int arg1, arg2;

    push(ep);
    while (!(IS_NIL(varlist))) {
	arg1 = car(varlist);
	arg2 = car(arglist);
	assocsym(arg1, arg2);
	varlist = cdr(varlist);
	arglist = cdr(arglist);
    }
}

void unbind(void)
{
    ep = pop();
}


int evlis(int addr)
{
    int car_addr, cdr_addr;

    argpush(addr);
    checkgbc();
    if (IS_NIL(addr)) {
	argpop();
	return (addr);
    } else {
	car_addr = eval(car(addr));
	argpush(car_addr);
	cdr_addr = evlis(cdr(addr));
	argpop();
	argpop();
	return (cons(car_addr, cdr_addr));
    }
}


//-------error------
void error(int errnum, char *fun, int arg)
{
    switch (errnum) {
    case CANT_FIND_ERR:{
	    printf("%s can't find difinition of ", fun);
	    print(arg);
	    break;
	}

    case CANT_READ_ERR:{
	    printf("%s can't read of ", fun);
	    break;
	}

    case CANT_OPEN_ERR:{
	    printf("%s can't open ", fun);
        print(arg);
	    break;
	}


    case ILLEGAL_OBJ_ERR:{
	    printf("%s got an illegal object ", fun);
	    print(arg);
	    break;
	}

    case ARG_SYM_ERR:{
	    printf("%s require symbol but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_INT_ERR:{
	    printf("%s require integer but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_NUM_ERR:{
	    printf("%s require number but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_STR_ERR:{
	    printf("%s require string but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_LIS_ERR:{
	    printf("%s require list but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_LEN0_ERR:{
	    printf("%s require 0 arg but got %d", fun, length(arg));
	    break;
	}

    case ARG_LEN1_ERR:{
	    printf("%s require 1 arg but got %d", fun, length(arg));
	    break;
	}

    case ARG_LEN2_ERR:{
	    printf("%s require 2 args but got %d", fun, length(arg));
	    break;
	}

    case ARG_LEN3_ERR:{
	    printf("%s require 3 args but got %d", fun, length(arg));
	    break;
	}

    case MALFORM_ERR:{
	    printf("%s got malformed args ", fun);
	    print(arg);
	    break;
	}
    }
    printf("\n");
    input_stream = stdin;
    longjmp(buf, 1);
}

void checkarg(int test, char *fun, int arg)
{
    switch (test) {
    case INTLIST_TEST:
	if (isintlis(arg))
	    return;
	else
	    error(ARG_INT_ERR, fun, arg);
    case NUMLIST_TEST:
	if (isnumlis(arg))
	    return;
	else
	    error(ARG_NUM_ERR, fun, arg);
    case SYMBOL_TEST:
	if (symbolp(arg))
	    return;
	else
	    error(ARG_SYM_ERR, fun, arg);
    case NUMBER_TEST:
	if (numberp(arg))
	    return;
	else
	    error(ARG_NUM_ERR, fun, arg);
    case STRING_TEST:
    if (stringp(arg))
	    return;
	else
	    error(ARG_STR_ERR, fun, arg);
    case LIST_TEST:
	if (listp(arg))
	    return;
	else
	    error(ARG_LIS_ERR, fun, arg);
    case LEN0_TEST:
	if (length(arg) == 0)
	    return;
	else
	    error(ARG_LEN0_ERR, fun, arg);
    case LEN1_TEST:
	if (length(arg) == 1)
	    return;
	else
	    error(ARG_LEN1_ERR, fun, arg);
    case LEN2_TEST:
	if (length(arg) == 2)
	    return;
	else
	    error(ARG_LEN2_ERR, fun, arg);
    case LEN3_TEST:
	if (length(arg) == 3)
	    return;
	else
	    error(ARG_LEN3_ERR, fun, arg);
    case DEFLIST_TEST:
	if (isdeflis(arg))
	    return;
	else
	    error(ILLEGAL_OBJ_ERR, fun, arg);
    case SYMLIST_TEST:
	if (issymlis(arg))
	    return;
	else
	    error(ILLEGAL_OBJ_ERR, fun, arg);
    
    }
}


int isintlis(int arg)
{
    while (!(IS_NIL(arg)))
	if (integerp(car(arg)))
	    arg = cdr(arg);
	else
	    return (0);
    return (1);
}


int isnumlis(int arg)
{
    while (!(IS_NIL(arg)))
	if (integerp(car(arg)) || floatp(car(arg)))
	    arg = cdr(arg);
	else
	    return (0);
    return (1);
}

int isdeflis(int arg)
{
    while (!(IS_NIL(arg)))
	if (symbolp(car(car(arg))) && listp(cadr(car(arg))))
	    arg = cdr(arg);
	else
	    return (0);
    return (1);
}

int issymlis(int arg)
{
    while(!nullp(arg)){
        if(!symbolp(car(arg)))
            return(0);
        arg = cdr(arg);
    }
    return(1);
}



//--------builtin functions------------
//define subr
void defsubr(char *symname, int (*func)(int))
{
    bindfunc(symname, SUBR, func);
}

//define fsubr
void deffsubr(char *symname, int (*func)(int))
{
    bindfunc(symname, FSUBR, func);
}

void bindfunc(char *name, tag tag, int (*func)(int))
{
    int sym, val;

    sym = makesym(name);
    val = freshcell();
    SET_TAG(val, tag);
    SET_SUBR(val, func);
    SET_CDR(val, 0);
    SET_BIND(sym,val);
}

void bindmacro(int sym, int addr)
{
    int val1, val2;

    val1 = freshcell();
    SET_TAG(val1, EXPR);
    SET_BIND(val1, addr);
    SET_CDR(val1, 0);
    val2 = freshcell();
    SET_TAG(val2, MACRO);
    SET_BIND(val2, val1);
    SET_CDR(val2, 0);
    bindsym(sym, val2);
}

int bindmacro1(int addr)
{
    int val1, val2;

    val1 = freshcell();
    SET_TAG(val1, EXPR);
    SET_BIND(val1, addr);
    SET_CDR(val1, 0);
    val2 = freshcell();
    SET_TAG(val2, MACRO);
    SET_BIND(val2, val1);
    SET_CDR(val2, 0);
    return(val2);
}

void initsubr(void)
{
    defsubr("+", f_plus);
    defsubr("-", f_difference);
    defsubr("minus", f_minus);
    defsubr("*", f_times);
    defsubr("/", f_quotient);
    defsubr("divide", f_divide);
    defsubr("add1", f_add1);
    defsubr("sub1", f_sub1);
    defsubr("max", f_max);
    defsubr("min", f_min);
    defsubr("recip", f_recip);
    defsubr("remainder", f_remainder);
    defsubr("expt", f_expt);
    defsubr("quit", f_quit);
    defsubr("hdmp", f_heapdump);
    defsubr("car", f_car);
    defsubr("cdr", f_cdr);
    defsubr("cadr", f_cadr);
    defsubr("caar", f_caar);
    defsubr("cddr", f_cddr);
    defsubr("caddr", f_caddr);
    defsubr("cadddr", f_cadddr);
    defsubr("cons", f_cons);
    defsubr("list", f_list);
    defsubr("reverse", f_reverse);
    defsubr("length", f_length);
    defsubr("append", f_append);
    defsubr("nconc", f_nconc);
    defsubr("rplaca", f_rplaca);
    defsubr("rplacd", f_rplacd);
    defsubr("mapcar", f_mapcar);
    defsubr("mapcon", f_mapcon);
    defsubr("map", f_map);
    defsubr("eq", f_eq);
    defsubr("equal", f_equal);
    defsubr("null", f_nullp);
    defsubr("atom", f_atomp);
    defsubr("gbc", f_gbc);
    defsubr("read", f_read);
    defsubr("readc", f_readc);
    defsubr("eval", f_eval);
    defsubr("apply", f_apply);
    defsubr("print", f_print);
    defsubr("prin1", f_prin1);
    defsubr("princ", f_princ);
    defsubr("newline", f_newline);
    deffsubr("trace", f_trace);
    deffsubr("untrace", f_untrace);
    defsubr("gensym", f_gensym);
    defsubr("step", f_step);
    defsubr("putprop", f_putprop);
    defsubr("get", f_get);
    defsubr("greaterp", f_greaterp);
    defsubr("lessp", f_lessp);
    defsubr("zerop", f_zerop);
    defsubr("onep", f_onep);
    defsubr("minusp", f_minusp);
    defsubr("numberp", f_numberp);
    defsubr("fixp", f_fixp);
    defsubr("symbolp", f_symbolp);
    defsubr("listp", f_listp);
    defsubr("assoc", f_assoc);
    defsubr("member", f_member);
    defsubr("set", f_set);
    defsubr("not", f_not);
    defsubr("load", f_load);
    defsubr("edwin", f_edwin);
    defsubr("subst", f_subst);
    defsubr("functionp", f_functionp);
    defsubr("macrop", f_macrop);
    defsubr("explode", f_explode);
    defsubr("implode", f_implode);
    defsubr("call/cc", f_call_cc);
    defsubr("push", f_push);
    defsubr("pop", f_pop);
    defsubr("bind", f_bind);
    defsubr("unbind", f_unbind);
    defsubr("test", f_test);
    defsubr("test1", f_test1);

    deffsubr("quote", f_quote);
    deffsubr("set!", f_setq);
    deffsubr("define", f_define);
    deffsubr("lambda", f_lambda);
    deffsubr("defmacro", f_defmacro);
    deffsubr("macro", f_macro);
    deffsubr("progn", f_progn);
    deffsubr("prog", f_prog);
    deffsubr("return", f_return);
    deffsubr("cond", f_cond);
    deffsubr("and", f_and);
    deffsubr("or", f_or);

    int addr, addr1, res;
    res = NIL;
    addr = ep;
    while (!(nullp(addr))) {
	addr1 = caar(addr);
	res = cons(addr1, res);
	addr = cdr(addr);
    }
    bindsym(makesym("oblist"), res);
}

//---------calculation-------------------
int plus(int x, int y)
{
    if (integerp(x)) {
	if (integerp(y))
	    return (makeint(GET_INT(x) + GET_INT(y)));
	if (floatp(y))
	    return (makeflt(GET_INT(x) + GET_FLT(y)));
    } else if (floatp(x)) {
	if (integerp(y))
	    return (makeflt(GET_FLT(x) + GET_INT(y)));
	else if (floatp(y))
	    return (makeflt(GET_FLT(x) + GET_FLT(y)));

    }

    return (NIL);
}

int difference(int x, int y)
{
    if (integerp(x)) {
	if (integerp(y))
	    return (makeint(GET_INT(x) - GET_INT(y)));
	if (floatp(y))
	    return (makeflt(GET_INT(x) - GET_FLT(y)));
    } else if (floatp(x)) {
	if (integerp(y))
	    return (makeflt(GET_FLT(x) - GET_INT(y)));
	else if (floatp(y))
	    return (makeflt(GET_FLT(x) - GET_FLT(y)));

    }

    return (NIL);
}

int times(int x, int y)
{
    if (integerp(x)) {
	if (integerp(y))
	    return (makeint(GET_INT(x) * GET_INT(y)));
	if (floatp(y))
	    return (makeflt(GET_INT(x) * GET_FLT(y)));
    } else if (floatp(x)) {
	if (integerp(y))
	    return (makeflt(GET_INT(x) * GET_FLT(y)));
	else if (floatp(y))
	    return (makeflt(GET_FLT(x) * GET_FLT(y)));

    }

    return (NIL);
}


int quotient(int x, int y)
{
    if (integerp(x)) {
	if (integerp(y))
	    return (makeint(GET_INT(x) / GET_INT(y)));
	if (floatp(y))
	    return (makeflt(GET_INT(x) / GET_FLT(y)));
    } else if (floatp(x)) {
	if (integerp(y))
	    return (makeflt(GET_INT(x) / GET_FLT(y)));
	else if (floatp(y))
	    return (makeflt(GET_FLT(x) / GET_FLT(y)));

    }

    return (NIL);
}

int greaterp(int x, int y)
{
    if (integerp(x)) {
	if (integerp(y)) {
	    if (GET_INT(x) > GET_INT(y))
		return (1);
	    else
		return (0);
	} else if (floatp(y)) {
	    if (GET_INT(x) > GET_FLT(y))
		return (1);
	    else
		return (0);
	}
    } else if (floatp(x)) {
	if (integerp(y)) {
	    if (GET_FLT(x) > GET_INT(y))
		return (1);
	    else
		return (0);
	} else if (floatp(y)) {
	    if (GET_FLT(x) > GET_FLT(y))
		return (1);
	    else
		return (0);
	}
    }
    return (NIL);
}

int lessp(int x, int y)
{
    if (greaterp(y, x))
	return (1);
    else
	return (0);
}


int power(int x, int y)
{
    if (y == 1)
	return (x);
    else if (y % 2 == 0)
	return (power(x * x, y / 2));
    else
	return (x * power(x, y - 1));
}


//---------subr--------------------------

int f_plus(int arglist)
{
    int arg, res;

    checkarg(NUMLIST_TEST, "plus", arglist);
    res = makeint(0);
    while (!(IS_NIL(arglist))) {
	arg = car(arglist);
	arglist = cdr(arglist);
	res = plus(res, arg);
    }
    return (res);
}

int f_add1(int arglist)
{
    int arg1;

    checkarg(NUMLIST_TEST, "add1", arglist);
    arg1 = car(arglist);
    return (plus(arg1, makeint(1)));
}

int f_sub1(int arglist)
{
    int arg1;

    checkarg(NUMLIST_TEST, "sub1", arglist);
    arg1 = car(arglist);
    return (difference(arg1, makeint(1)));
}

int f_difference(int arglist)
{
    int arg1, arg2;
    checkarg(NUMLIST_TEST, "difference", arglist);
    checkarg(LEN2_TEST, "difference", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (difference(arg1, arg2));
}

int f_minus(int arglist)
{
    int arg1;

    checkarg(NUMLIST_TEST, "minus", arglist);
    checkarg(LEN1_TEST, "minus", arglist);
    arg1 = car(arglist);
    return (times(arg1, makeint(-1)));
}

int f_times(int arglist)
{
    int arg, res;

    checkarg(NUMLIST_TEST, "times", arglist);
    res = makeint(1);
    while (!(IS_NIL(arglist))) {
	arg = car(arglist);
	arglist = cdr(arglist);
	res = times(res, arg);
    }
    return (res);
}

int f_quotient(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "quotitent", arglist);
    checkarg(NUMLIST_TEST, "quotient", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (quotient(arg1,arg2));
}

int f_divide(int arglist)
{
    int arg1, arg2, q, m;

    checkarg(LEN2_TEST, "divide", arglist);
    checkarg(INTLIST_TEST, "divide", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    q = makeint(GET_INT(arg1) / GET_INT(arg2));
    m = makeint(GET_INT(arg1) % GET_INT(arg2));
    return (cons(q, cons(m, NIL)));
}

int f_max(int arglist)
{
    int res;

    checkarg(NUMLIST_TEST, "max", arglist);
    res = car(arglist);
    arglist = cdr(arglist);
    while (!(IS_NIL(arglist))) {
	if (greaterp(car(arglist), res))
	    res = car(arglist);
	arglist = cdr(arglist);
    }
    return (res);
}

int f_min(int arglist)
{
    int res;

    checkarg(NUMLIST_TEST, "min", arglist);
    res = car(arglist);
    arglist = cdr(arglist);
    while (!(IS_NIL(arglist))) {
	if (lessp(car(arglist), res))
	    res = car(arglist);
	arglist = cdr(arglist);
    }
    return (res);
}


int f_recip(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "recip", arglist);
    checkarg(NUMLIST_TEST, "recip", arglist);
    arg1 = car(arglist);
    if (integerp(arg1))
	return (makeint(0));

    return (quotient(makeflt(1.0), arg1));
}

int f_remainder(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "remainder", arglist);
    checkarg(INTLIST_TEST, "remainder", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (makeint(GET_INT(arg1) % GET_INT(arg2)));
}

int f_expt(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "expt", arglist);
    checkarg(INTLIST_TEST, "expt", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if (GET_INT(arg2) == 0)
	return (makeint(1));
    else if (GET_INT(arg1) < 0)
	error(ILLEGAL_OBJ_ERR, "expt", arglist);
    return (makeint(power(GET_INT(arg1), GET_INT(arg2))));
}




int f_quit(int arglist)
{
    int addr;

    checkarg(LEN0_TEST, "quit", arglist);
    for (addr = 0; addr < HEAPSIZE; addr++) {
	free(heap[addr].name);
    }
    printf("- Dedicated to the memory of TIScheme. -\n");
    longjmp(buf, 2);
}

int f_heapdump(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "hdmp", arglist);
    arg1 = GET_INT(car(arglist));
    heapdump(arg1, arg1 + 10);
    return (T);
}

int f_car(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "car", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "car", arg1);
    return (car(arg1));
}

int f_cdr(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "cdr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "cdr", arg1);
    return (cdr(arg1));
}

int f_caar (int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "caar", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "caar", arg1);
    return (caar(arg1));
}

int f_cadr (int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "cadr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "cadr", arg1);
    return (cadr(arg1));
}

int f_cddr (int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "cddr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "cddr", arg1);
    return (cddr(arg1));
}

int f_caddr (int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "caddr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "caddr", arg1);
    return (caddr(arg1));
}

int f_cadddr (int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "cadddr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "cadddr", arg1);
    return (cadddr(arg1));
}

int f_cons(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "cons", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (cons(arg1, arg2));
}

int f_eq(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "eq", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if (eqp(arg1, arg2))
	return (T);
    else
	return (NIL);
}

int f_equal(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "equal", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if (equalp(arg1, arg2))
	return (T);
    else
	return (NIL);
}

int f_nullp(int arglist)
{
    int arg;

    checkarg(LEN1_TEST, "null", arglist);
    arg = car(arglist);
    if (nullp(arg))
	return (T);
    else
	return (NIL);
}

int f_atomp(int arglist)
{
    int arg;

    checkarg(LEN1_TEST, "atom", arglist);
    arg = car(arglist);
    if (atomp(arg))
	return (T);
    else
	return (NIL);
}

int f_length(int arglist)
{
    checkarg(LEN1_TEST, "length", arglist);
    checkarg(LIST_TEST, "length", car(arglist));
    return (makeint(length(car(arglist))));
}

int f_list(int arglist)
{
    return (list(arglist));
}

int f_reverse(int arglist)
{
    int arg1, res;
    checkarg(LEN1_TEST, "reverse", arglist);
    checkarg(LIST_TEST, "reverse", car(arglist));

    arg1 = car(arglist);
    res = NIL;
    while (!nullp(arg1)) {
	res = cons(car(arg1), res);
	arg1 = cdr(arg1);
    }
    return (res);
}

int f_assoc(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "assoc", arglist);
    checkarg(SYMBOL_TEST, "assoc", car(arglist));
    checkarg(LIST_TEST, "assoc", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (assoc(arg1, arg2));
}

int f_member(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "member", arglist);
    checkarg(LIST_TEST, "member", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (member(arg1, arg2));
}


int f_append(int arglist)
{
    if(nullp(arglist))
        return(NIL);
    return (append(car(arglist), f_append(cdr(arglist))));
}

int f_nconc(int arglist)
{
    checkarg(LEN2_TEST, "nconc", arglist);
    checkarg(LIST_TEST, "nconc", car(arglist));
    return (nconc(car(arglist), cadr(arglist)));
}

int f_rplaca(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "rplaca", arglist);
    checkarg(LIST_TEST, "rplaca", car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    SET_CAR(arg1, arg2);
    return (arg1);
}

int f_rplacd(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "rplacd", arglist);
    checkarg(LIST_TEST, "rplacd", car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    SET_CDR(arg1, arg2);
    return (arg1);
}


int f_symbolp(int arglist)
{
    checkarg(LEN1_TEST, "symbolp", arglist);
    if (symbolp(car(arglist)))
	return (T);
    else
	return (NIL);
}

int f_numberp(int arglist)
{
    checkarg(LEN1_TEST, "numberp", arglist);
    if (numberp(car(arglist)))
	return (T);
    else
	return (NIL);
}

int f_fixp(int arglist)
{
    checkarg(LEN1_TEST, "fixp", arglist);
    if (fixp(car(arglist)))
	return (T);
    else
	return (NIL);
}


int f_listp(int arglist)
{
    if (listp(car(arglist)))
	return (T);
    else
	return (NIL);
}

int f_onep(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "onep", arglist);
    checkarg(NUMLIST_TEST, "onep", arglist);
    arg1 = car(arglist);

    if (integerp(arg1) && GET_INT(arg1) == 1)
	return (T);
    else if (floatp(arg1) && fabs(GET_FLT(arg1) - 1.0) < DBL_MIN)
	return (T);
    else
	return (NIL);
}

int f_zerop(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "zerop", arglist);
    checkarg(NUMLIST_TEST, "zerop", arglist);
    arg1 = car(arglist);

    if (integerp(arg1) && GET_INT(arg1) == 0)
	return (T);
    else if (floatp(arg1) && fabs(GET_FLT(arg1)) < DBL_MIN)
	return (T);
    else
	return (NIL);
}


int f_minusp(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "minusp", arglist);
    checkarg(NUMLIST_TEST, "minusp", arglist);
    arg1 = car(arglist);

    if (lessp(arg1, makeint(0)))
	return (T);
    else
	return (NIL);
}



int f_lessp(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "lessp", arglist);
    checkarg(NUMLIST_TEST, "lessp", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (lessp(arg1, arg2))
	return (T);
    else
	return (NIL);
}

int f_greaterp(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "greaterp", arglist);
    checkarg(NUMLIST_TEST, "greaterp", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (greaterp(arg1, arg2))
	return (T);
    else
	return (NIL);
}


int f_gbc(int arglist)
{

    if (car(arglist) == T)
	gbc_flag = 1;
    else if (car(arglist) == NIL)
	gbc_flag = 0;
    else if (car(arglist) == makeint(1)) {
	printf("execute gc!\n");
	gbc();
    }
    return (T);
}

int f_read(int arglist)
{
    checkarg(LEN0_TEST, "read", arglist);
    return (read());
}

int f_readc(int arglist)
{
    char c,str[5];
    checkarg(LEN0_TEST, "readc", arglist);
    memset(str,0,5);
    c = fgetc(input_stream);
    str[0] = c;
    return (makesym(str));
}

int f_print(int arglist)
{
    checkarg(LEN1_TEST, "print", arglist);
    print(car(arglist));
    printf("\n");
    return (T);
}

int f_prin1(int arglist)
{
    checkarg(LEN1_TEST, "prin1", arglist);
    print(car(arglist));
    return (T);
}

int f_princ(int arglist)
{
    checkarg(LEN1_TEST, "princ", arglist);
    princ(car(arglist));
    return (T);
}


int f_newline(int arglist)
{
    checkarg(LEN0_TEST, "newline", arglist);
    printf("\n");
    return(T);
}

int f_trace(int arglist)
{
    int arg1;
    checkarg(SYMBOL_TEST,"trace",car(arglist));
    arg1 = car(arglist);
    SET_TR(arg1,1);
    return(T);
}

int f_untrace(int arglist)
{
    int arg1;
    checkarg(SYMBOL_TEST,"untrace",car(arglist));
    arg1 = car(arglist);
    SET_TR(arg1,0);
    return(T);
}

int f_gensym(int arglist)
{   
    char gsym[SYMSIZE];
    checkarg(LEN0_TEST,"gensym",arglist);
    sprintf(gsym,"G%05d",gennum);
    gennum++;
    return(makesym(gsym));
}

int f_step(int arglist){
    int arg1;

    checkarg(LEN1_TEST,"step",arglist);
    arg1 = car(arglist);
    if(arg1 == T){
        oblist_len = length(ep);
        step_flag = 1;
    }
    else if(arg1 == NIL)
        step_flag = 0;
    else 
        error(ILLEGAL_OBJ_ERR,"step",arg1);

    return(T);
}


int f_putprop(int arglist)
{
    int arg1,arg2,arg3,plist,res;
    checkarg(LEN3_TEST,"putprop",arglist);
    checkarg(SYMBOL_TEST,"putprop",car(arglist));
    checkarg(SYMBOL_TEST,"putprop",cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = caddr(arglist);
    plist = GET_CDR(arg1);
    res = assoc(arg2,plist);
    if(res == NO)
        SET_CDR(arg1,cons(cons(arg2,arg3),plist));
    else
        SET_CDR(res,arg3);
    return(T);
}

int f_defprop(int arglist)
{
    int arg1,arg2,arg3;
    checkarg(LEN3_TEST,"defprop",arglist);
    checkarg(SYMBOL_TEST,"defprop",car(arglist));
    checkarg(SYMBOL_TEST,"defprop",cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = caddr(arglist);
    SET_CDR(arg1,cons(cons(arg2,arg3),NIL));
    return(T);
}


int f_get(int arglist)
{
    int arg1,arg2,res;
    checkarg(LEN2_TEST,"get",arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    res = assoc(arg2,GET_CDR(arg1));
    if(res == NO)
        return(NIL);
    
    return(cdr(res));
}


int f_eval(int arglist)
{
    checkarg(LEN1_TEST, "eval", arglist);
    return (eval_cps(car(arglist)));
}

int f_apply(int arglist)
{
    checkarg(LEN2_TEST, "apply", arglist);
    checkarg(LIST_TEST, "apply", cadr(arglist));
    int arg1, arg2;

    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if(functionp(arg1) || subrp(arg1) || fsubrp(arg1))
        arg1 = GET_BIND(arg1);
    else if(lambdap(arg1))
        arg1 = eval_cps(arg1);
    else 
        error(ILLEGAL_OBJ_ERR,"apply",arg1);
    return (apply(arg1, arg2));
}

int f_mapcar(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "mapcar", arglist);
    checkarg(LIST_TEST, "mapcar", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if(functionp(arg1) || subrp(arg1) || fsubrp(arg1))
        arg1 = GET_BIND(arg1);
    else if(lambdap(arg1))
        arg1 = eval(arg1);
    else 
        error(ILLEGAL_OBJ_ERR,"mapcar",arg1);
    return (mapcar(arg2, arg1));

}

int f_mapcon(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "mapcon", arglist);
    checkarg(LIST_TEST, "mapcon", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if(functionp(arg1) || subrp(arg1) || fsubrp(arg1))
        arg1 = GET_BIND(arg1);
    else if(lambdap(arg1))
        arg1 = eval(arg1);
    else 
        error(ILLEGAL_OBJ_ERR,"mapcon",arg1);
    return (mapcon(arg2, arg1));

}

int f_map(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "map", arglist);
    checkarg(LIST_TEST, "map", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if(functionp(arg1) || subrp(arg1) || fsubrp(arg1))
        arg1 = GET_BIND(arg1);
    else if(lambdap(arg1))
        arg1 = eval(arg1);
    else 
        error(ILLEGAL_OBJ_ERR,"map",arg1);
    return (map(arg2, arg1));

}


int f_set(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "set", arglist);
    checkarg(SYMBOL_TEST, "set", car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    bindsym(arg1, arg2);
    return (T);
}

int f_not(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "set", arglist);

    arg1 = car(arglist);
    if (arg1 == NIL)
	return (T);
    else
	return (NIL);
}

int subst(int x, int y, int lis)
{
    if(nullp(lis))
        return(NIL);
    else if(eqp(y,lis))
        return(x);
    else if(atomp(lis))
        return(lis);
    else return(cons(subst(x,y,car(lis)),
                    (subst(x,y,cdr(lis)))));
}

int f_subst(int arglist)
{
    int arg1,arg2,arg3;

    checkarg(LIST_TEST,"subst",caddr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = caddr(arglist);

    return(subst(arg1,arg2,arg3));
}


//--FSUBR-----------
int f_quote(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "quote", arglist);
    arg1 = car(arglist);
    return (arg1);
}

int f_setq(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "setq", arglist);
    checkarg(SYMBOL_TEST, "setq", car(arglist));
    arg1 = car(arglist);
    arg2 = eval(cadr(arglist));
    bindsym(arg1, arg2);
    return (T);
}

         
void bindfunc1(int sym, int type, int addr){
    int val;
	
	val = freshcell();
    SET_TAG(val,type);
    SET_BIND(val,addr);
    SET_CDR(val,0);
    SET_BIND(sym,val);
}


int f_define(int arglist){
	int arg1,arg2;
    
    arg1 = NIL;
    if(listp(car(arglist))){
    arg1 = caar(arglist); // function name
    arg2 = cons(cdar(arglist),cdr(arglist)); // argument + body
    bindfunc1(arg1,EXPR,arg2);
    } else if(symbolp(car(arglist)) && lambdap(cadr(arglist))){
    checkarg(LIST_TEST, "define" ,cadr(arglist));
    arg1 = car(arglist);  // function name
    arg2 = cadr(arglist); //lambda exp
    SET_BIND(arg1,eval(arg2));
    } else if(symbolp(car(arglist))){
    arg1 = car(arglist); //variable name
    arg2 = cadr(arglist); //value
    bindsym(arg1,arg2);
    }
    return(arg1);
}


int f_lambda(int arglist)
{

    checkarg(LIST_TEST, "lambda", car(arglist));
    checkarg(LIST_TEST, "lambda", cdr(arglist));
    return (makefunc(arglist));
}

int f_defmacro(int arglist)
{
    int arg1, arg2;

    checkarg(LEN3_TEST, "defmacro", arglist);
    checkarg(SYMBOL_TEST, "defmacro", car(arglist));
    checkarg(LIST_TEST, "defmacro", cadr(arglist));
    checkarg(LIST_TEST, "defmacro", caddr(arglist));
    arg1 = car(arglist);
    arg2 = cdr(arglist);
    bindmacro(arg1, arg2);
    return (T);
}

int f_macro(int arglist)
{
    checkarg(LEN2_TEST, "macro", arglist);
    checkarg(LIST_TEST, "macro", car(arglist));
    return(bindmacro1(arglist));
}

int f_if(int arglist)
{
    int arg1, arg2, arg3;

    checkarg(LEN3_TEST, "if", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = car(cdr(cdr(arglist)));

    if (!(nullp(eval(arg1))))
	return (eval(arg2));
    else
	return (eval(arg3));
}

int f_cond(int arglist)
{
    int arg1, arg2, arg3;

    if (nullp(arglist))
	return (NIL);

    arg1 = car(arglist);
    checkarg(LIST_TEST, "cond", arg1);
    arg2 = car(arg1);
    arg3 = cdr(arg1);

    if (!(nullp(eval(arg2))))
	return (f_progn(arg3));
    else
	return (f_cond(cdr(arglist)));
}

int f_progn(int arglist)
{
    int res;

    res = NIL;
    while (!(nullp(arglist))) {
	res = eval(car(arglist));
	arglist = cdr(arglist);
    }
    return (res);
}

int find_label(int label, int prog)
{
    while (!nullp(prog)) {
	if (eqp(car(prog), label))
	    return (prog);
	prog = cdr(prog);
    }
    error(ILLEGAL_OBJ_ERR, "prog", label);

    return (NIL);
}

int f_prog(int arglist)
{
    int arg1, arg2, res, prog, save;

    checkarg(SYMLIST_TEST, "prog", car(arglist));
    arg1 = car(arglist);
    arg2 = cdr(arglist);
    prog = arg2;
    return_flag = 0;
    save = ep;
    res = NIL;
    while (!nullp(arg1)) {
	ep = cons(cons(car(arg1), NIL),ep);
	arg1 = cdr(arg1);
    }
    while (!nullp(arg2)) {
	res = car(arg2);
	if (symbolp(res)) {
	    goto skip;
	} else if (listp(res) && eqp(car(res), makesym("go"))) {
	    arg2 = (find_label(cadr(res), prog));
	    goto skip;
	}

	res = eval(res);
	if (return_flag) {
	    ep = save;
	    return (res);
	}
      skip:
	arg2 = cdr(arg2);
    }
    ep = save;
    return (res);
}

int f_return(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "return", arglist);
    arg1 = car(arglist);
    return_flag = 1;
    return (eval(arg1));
}


int f_and(int arglist)
{
    int res;

    res = NIL;
    while (!nullp(arglist)) {
	res = eval(car(arglist));
	if (res == NIL)
	    return (NIL);
	arglist = cdr(arglist);
    }
    return (res);
}

int f_or(int arglist)
{
    int res;

    while (!nullp(arglist)) {
	res = eval(car(arglist));
	if (res != NIL)
	    return (res);
	arglist = cdr(arglist);
    }
    return (NIL);
}

int f_load(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "load", arglist);
    checkarg(STRING_TEST, "load", car(arglist));
    arg1 = car(arglist);

    input_stream = fopen(GET_NAME(arg1), "r");
    if (!input_stream) 
	error(CANT_OPEN_ERR,"load",arg1);
    

    int exp;
    while (1) {
	exp = read();
	if (exp == EOF)
	    break;
	eval_cps(exp);
    }
    fclose(input_stream);
    input_stream = stdin;
    return (T);
}

int f_edwin(int arglist)
{
    int arg1,res;
    char str[SYMSIZE];

    arg1 = car(arglist);
    char *ed;
	if ((ed = getenv("EDITOR")) == NULL) {
	    strcpy(str,"edlis");
	} else 
        strcpy(str,ed);
    
    strcat(str," ");
    strcat(str,GET_NAME(arg1));
    res = system(str);
    if (res == -1)
	error(CANT_OPEN_ERR, "ledit", arg1);
    f_load(arglist);
    return (T);
}

int f_functionp(int arglist)
{
    checkarg(LEN1_TEST,"functionp",arglist);
    if(functionp(car(arglist)))
        return(T);
    else 
        return(NIL);
}

int f_macrop(int arglist)
{
    checkarg(LEN1_TEST,"macrop",arglist);
    if(macrop(car(arglist)))
        return(T);
    else 
        return(NIL);
}

int f_explode(int arglist)
{
    int arg1,pos,temp,res;
    char c,str[SYMSIZE],ch[5];

    checkarg(LEN1_TEST,"explode",arglist);
    checkarg(SYMBOL_TEST,"explode",car(arglist));
    arg1 = car(arglist);
    memset(str,0,SYMSIZE);
    strcpy(str,GET_NAME(arg1));
    temp = NIL;
    pos = 0;
    c = str[pos];
    while(c != 0){
        ch[0] = c;
        ch[1] = 0;
        pos++;
        c = str[pos];
        temp = cons(makesym(ch),temp);
    }
    res = NIL;
    while(temp != NIL){
        res = cons(car(temp),res);
        temp = cdr(temp);
    }
    return(res);
}

int f_implode(int arglist)
{
    int arg1;
    char str[SYMSIZE];

    checkarg(SYMLIST_TEST,"implode",car(arglist));
    arg1 = car(arglist);
    memset(str,0,SYMSIZE);
    while(arg1 != NIL){
        strcat(str,GET_NAME(car(arg1)));
        arg1 = cdr(arg1);
    }
    return(makesym(str));
}

int f_call_cc(int arglist)
{
    int arg1,cont;
    checkarg(LEN1_TEST,"call/cc",arglist);
    arg1 = car(arglist);
    cont = cons(makesym("lambda"),cons(list1(makesym("cont")),cp));
    // (lambda (cont) continuation)
    cont = eval(cont);
    return (apply(arg1,list1(cont)));
}

int f_push(int arglist)
{
    cps_push(acc);
    return(T);
}

int f_pop(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    return(cps_pop(GET_INT(arg1)));
}

int f_bind(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    cps_bind(arg1);
    return(T);
}

int f_unbind(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    cps_unbind(GET_INT(arg1));
    return(acc);
}

int f_test(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    return(transfer(arg1));
}

int f_test1(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    return(eval_cps(arg1));
}

//--------quasi-quote---------------
int quasi_transfer1(int x)
{
    if (nullp(x))
	return (NIL);
    else if (atomp(x))
	return (list2(makesym("quote"), x));
    else if (listp(x) && eqp(caar(x), makesym("unquote")))
	return (list3(makesym("cons"), cadar(x), quasi_transfer1(cdr(x))));
    else if (listp(x) && eqp(caar(x), makesym("unquote-splicing")))
	return (list3
		(makesym("append"), cadar(x), quasi_transfer1(cdr(x))));
    else
	return (list3
		(makesym("cons"), quasi_transfer1(car(x)),
		 quasi_transfer1(cdr(x))));
}

int list1(int x)
{
    return (cons(x, NIL));
}

int list2(int x, int y)
{
    return (cons(x, cons(y, NIL)));
}

int list3(int x, int y, int z)
{
    return (cons(x, cons(y, cons(z, NIL))));
}


int quasi_transfer2(int x, int n)
{
    if (nullp(x))
	return (NIL);
    else if (atomp(x))
	return (list2(makesym("quote"), x));
    else if (listp(x) && eqp(car(x), makesym("unquote")) && n == 0)
	return (cadr(x));
    else if (listp(x) && eqp(car(x), makesym("unquote-splicing"))
	     && n == 0)
	return (cadr(x));
    else if (listp(x) && eqp(car(x), makesym("quasi-quote")))
	return (list3(makesym("list"),
		      list2(makesym("quote"), makesym("quasi-quote")),
		      quasi_transfer2(cadr(x), n + 1)));
    else if (listp(x) && eqp(caar(x), makesym("unquote")) && n == 0)
	return (list3
		(makesym("cons"), cadar(x), quasi_transfer2(cdr(x), n)));
    else if (listp(x) && eqp(caar(x), makesym("unquote-splicing"))
	     && n == 0)
	return (list3
		(makesym("append"), cadar(x),
		 quasi_transfer2(cdr(x), n - 1)));
    else if (listp(x) && eqp(caar(x), makesym("unquote")))
	return (list3(makesym("cons"),
		      list3(makesym("list"),
			    list2(makesym("quote"), makesym("unquote")),
			    quasi_transfer2(cadar(x), n - 1)),
		      quasi_transfer2(cdr(x), n)));
    else if (listp(x) && eqp(caar(x), makesym("unquote-splicing")))
	return (list3(makesym("cons"),
		      list3(makesym("list"),
			    list2(makesym("quote"),
				  makesym("unquote-splicing")),
			    quasi_transfer2(cadar(x), n - 1)),
		      quasi_transfer2(cdr(x), n)));
    else
	return (list3
		(makesym("cons"), quasi_transfer2(car(x), n),
		 quasi_transfer2(cdr(x), n)));

}
