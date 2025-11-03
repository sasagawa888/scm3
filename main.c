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

double version = 1.50;
cell heap[HEAPSIZE];
int stack[STACKSIZE];
int argstk[STACKSIZE];
int protect[STACKSIZE];
token stok = { GO, OTHER };

jmp_buf buf;
int cell_hash_table[HASHTBSIZE];

int input_stream;
int output_stream;
int gbc_flag = 0;
int return_flag = 0;
int step_flag = 0;
int gennum = 1;
int ctrl_c_flag;
int letrec_flag = 0;
int display_flag = 0;
int continuation_variable;
int continutation_entity;

void signal_handler_c(int signo)
{
    ctrl_c_flag = 1;
}

int main(int argc, char *argv[])
{

    printf("Scheme R3RS ver %.2f\n", version);
    input_stream = STDIN;
    output_stream = STDOUT;
    initcell();
    initsubr();
    signal(SIGINT, signal_handler_c);

    int ret = setjmp(buf);

  repl:
    if (ret == 0)
	while (1) {
	    printf("> ");
	    fflush(stdout);
	    fflush(stdin);
	    cp = NIL;
	    cp1 = NIL;
	    sp = 0;
	    sp_cps = NIL;
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
    ep = NIL;
    makesym("nil");
    makebool("#t");
    makebool("#f");
    makestm(stdin,INPUT,"stdin");
    makestm(stdout,OUTPUT,"stdout");

    sp = 0;
    ap = 0;
    pp = 0;
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
    case BOOL:
	printf("BOOL   ");
	break;
    case CHAR:
	printf("CHAR   ");
	break;
    case LIS:
	printf("LIS    ");
	break;
    case VEC:
	printf("VEC    ");
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
    case CONT:
	printf("CONT   ");
	break;
    case PROM:
	printf("PROMISE");
	break;
    case STM:
	printf("STREAM ");
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
    if (addr < 0 || addr >= HEAPSIZE)
	return;
    if (integerp(addr))
	return;
    if (USED_CELL(addr))
	return;

    MARK_CELL(addr);
    if (addr == T || addr == NIL || addr == TRUE || addr == FAIL)
	return;
    if (addr == STDIN || addr == STDOUT)
    return;


    if (listp(addr)) {
	markcell(car(addr));
	markcell(cdr(addr));
	return;
    }

    if (symbolp(addr)) {
	markcell(GET_BIND(addr));
	return;
    }

    if (IS_SUBR(addr) || IS_FSUBR(addr)) {
	markcell(GET_BIND(addr));
	return;
    }

    if (IS_EXPR(addr)) {
	markcell(GET_BIND(addr));
	markcell(GET_CDR(addr));	//ep envinronment
	return;
    }



}

void gbcmark(void)
{
    int addr, i;

    markcell(TRUE);
    markcell(FAIL);
    markcell(STDIN);
    markcell(STDOUT);
    markcell(cp);		// continuation
    markcell(cp1);
    markcell(sp_cps);		//cps stack 
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

    // Mark the cells that are protected.
    for (i = 0; i < pp; i++)
	markcell(protect[i]);


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
    SET_REC(addr,0);
}

// If the number of free cells falls below a certain threshold, trigger GBC.
void checkgbc(void)
{
    if (ctrl_c_flag) {
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
    if (IS_FLT(addr) || IS_STR(addr) || IS_SYMBOL(addr) || IS_BOOL(addr)
	|| IS_CHAR(addr) || IS_VECTOR(addr) || IS_SUBR(addr)
	|| IS_FSUBR(addr)
	|| IS_EXPR(addr))
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

int zerop(int addr)
{
    if (integerp(addr) && GET_INT(addr) == 0)
	return (1);
    else if (floatp(addr) && fabs(GET_FLT(addr)) < DBL_MIN)
	return (1);
    else 
    return(0);
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

int rationalp(int addr)
{
    if (integerp(addr))
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

int booleanp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_BOOL(addr))
	return (1);
    else
	return (0);
}

int characterp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_CHAR(addr))
	return (1);
    else
	return (0);
}



int continuationp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_CONT(addr))
	return (1);
    else
	return (0);
}

int let_loop_p(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (GET_CAR(addr) != NIL)
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

int vectorp(int addr)
{
    if (addr >= HEAPSIZE || addr < 0)
	return (0);
    else if (IS_VECTOR(addr))
	return (1);
    else
	return (0);
}


int pairp(int addr)
{
    if (listp(addr) && addr != NIL)
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



int numeqp(int addr1, int addr2)
{
    if (integerp(addr1) && integerp(addr2) && addr1 == addr2)
	return (1);
    else if (floatp(addr1) && floatp(addr2)
	     && GET_FLT(addr1) - GET_FLT(addr2) < DBL_MIN)
	return (1);
    else if (integerp(addr1) && floatp(addr2)){
        if ((double)GET_INT(addr1) - GET_FLT(addr2) < DBL_MIN)
            return(1);
        else 
            return(0);
    }
     else if (integerp(addr2) && floatp(addr1)){
        if ((double)GET_INT(addr2) - GET_FLT(addr1) < DBL_MIN)
            return(1);
        else 
            return(0);
    }
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

int stringeqp(int addr1, int addr2)
{
    if (stringp(addr1) && stringp(addr2)
	&& strcmp(GET_NAME(addr1), GET_NAME(addr2)) == 0)
	return (1);
    else
	return (0);
}

int chareqp(int addr1, int addr2)
{
    if (characterp(addr1) && characterp(addr2)
	&& strcmp(GET_NAME(addr1), GET_NAME(addr2)) == 0)
	return (1);
    else
	return (0);
}

int eqvp(int addr1, int addr2)
{
    if (eqp(addr1, addr2) || numeqp(addr1, addr2) || chareqp(addr1, addr2))
	return (1);
    else if (stringp(addr1) && stringp(addr2) && eqp(addr1,addr2))
    return (1);
    else
	return (0);
}

int equalp(int x, int y)
{

    if (nullp(x) && nullp(y))
    return (1);
    else if (stringp(x) && stringp(y) && stringeqp(x,y))
    return (1);
    else if (atomp(x) && atomp(y))
	return (eqvp(x, y));
    else if (equalp(car(x), car(y)) && equalp(cdr(x), cdr(y)))
	return (1);
    return (0);
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

int experp(int addr)
{

    if (symbolp(addr))
	return (IS_EXPR(findsym(addr)));
    else
	return (0);
}

int promisep(int addr)
{
    if (addr >= 0 && addr < HEAPSIZE && IS_PROM(addr))
    return(1);
    else 
    return(0);
}


int lambdap(int addr)
{
    if (listp(addr) && car(addr) == makesym("lambda"))
	return (1);
    else
	return (0);
}

int in_recur_p(int addr)
{
    if (GET_REC(GET_BIND(addr)))
	return (1);
    else
	return (0);
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
	return (FAIL);
    else if (equalp(x, car(lis)))
	return (lis);
    else
	return (member(x, cdr(lis)));
}

int memq(int x, int lis)
{
    if (nullp(lis))
	return (FAIL);
    else if (eqp(x, car(lis)))
	return (lis);
    else
	return (memq(x, cdr(lis)));
}


int memv(int x, int lis)
{
    if (nullp(lis))
	return (FAIL);
    else if (eqvp(x, car(lis)))
	return (lis);
    else
	return (memv(x, cdr(lis)));
}


int reverse(int addr)
{
    int res;
    res = NIL;
    while (!nullp(addr)) {
	res = cons(car(addr), res);
	addr = cdr(addr);
    }
    return (res);
}

int list_tail(int x, int k)
{
    if (k == 0)
	return (x);
    else
	return (list_tail(cdr(x), k - 1));
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

int map(int lis, int fun)
{
    if (nullp(lis))
	return (NIL);
    else
	return (cons(apply_cps(fun, list1(car(lis))), map(cdr(lis), fun)));
}

int for_each(int lis, int fun)
{
    if (nullp(lis))
	return (NIL);
    else
	return (apply_cps(fun, list1(car(lis))), for_each(cdr(lis), fun));
}


int copy(int addr)
{
    if (nullp(addr))
	return (NIL);
    else if (atomp(addr))
	return (addr);
    else
	return (cons(copy(car(addr)), copy(cdr(addr))));
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

int res, i, *vec;


int makebool(char *name)
{
    int addr;

    addr = freshcell();
    SET_TAG(addr, BOOL);
    SET_NAME(addr, name);
    return (addr);
}

int makechar(char *name)
{
    int addr;

    addr = freshcell();
    SET_TAG(addr, CHAR);
    SET_NAME(addr, name);
    return (addr);
}


int makevec(int addr)
{
    int n, i, *vec;
    n = length(addr);
    res = freshcell();
    vec = (int *) malloc(sizeof(int) * n);
    SET_TAG(res, VEC);
    SET_VEC(res, vec);
    for (i = 0; i < n; i++) {
	SET_VEC_ELT(res, i, car(addr));
	addr = cdr(addr);
    }
    SET_CDR(res, n);
    return (res);
}


int makesym(char *name)
{
    int index, res, pos;
    char name1[SYMSIZE];

    memset(name1, 0, SYMSIZE);
    pos = 0;
    while (name[pos] != 0) {
	if (isupper(name[pos]))
	    name1[pos] = name[pos] + 32;
	else
	    name1[pos] = name[pos];
	pos++;
    }

    index = hash(name1);
    if ((res = getsym(name1, index)) != 0)
	return (res);
    else
	return (addsym(name1, index));
}


int makefunc(int addr)
{
    int val;

    val = freshcell();
    SET_TAG(val, EXPR);
    SET_BIND(val, addr);
    SET_CDR(val, copy(ep));
    return (val);
}

int makecont(void)
{
    int val;

    val = freshcell();
    SET_TAG(val, CONT);
    SET_BIND(val, cp);
    SET_CAR(val, sp_cps);
    SET_CDR(val, ep);
    return (val);
}

int makepromise(int addr)
{
    int val;

    val = freshcell();
    SET_TAG(val, PROM);
    SET_BIND(val, addr);
    SET_CAR(val,NIL);
    SET_CDR(val,NIL);
    return (val);
}

int makestm(FILE *stream, int prop, char *name)
{
    int val;

    val = freshcell();
    SET_TAG(val, STM);
    SET_STM(val, stream);
    SET_CDR(val,prop);
    SET_NAME(val,name)
    return (val);
}


int ci_str(int addr)
{
    char str[SYMSIZE];
    int i;
    memset(str, 0, SYMSIZE);
    strcpy(str, GET_NAME(addr));
    i = 0;
    while (str[i] != 0) {
	if (islower(str[i]))
	    str[i] = str[i] - 32;

	i++;
    }
    return (makestr(str));
}


int ci_char(int addr)
{
    char str[SYMSIZE];
    int i;
    memset(str, 0, SYMSIZE);
    strcpy(str, GET_NAME(addr));
    i = 0;
    while (str[i] != 0) {
	if (islower(str[i]))
	    str[i] = str[i] - 32;

	i++;
    }
    return (makechar(str));
}


int exact_to_inexact(int x)
{
    if (integerp(x))
	return (makeflt(GET_INT(x)));
    else
	return (x);
}

int inexact_to_exact(int x)
{
    if (floatp(x))
	return (makeint((double)GET_FLT(x)));
    else
	return (x);
}



//-------for CPS--------------------
void push_cps(int addr)
{
    sp_cps = cons(addr, sp_cps);
}

int pop_cps(void)
{
    int res;
    res = car(sp_cps);
    sp_cps = cdr(sp_cps);
    return (res);
}

int pops_cps(int n)
{
    int res;
    res = NIL;
    while (n > 0) {
	res = cons(car(sp_cps), res);
	sp_cps = cdr(sp_cps);
	n--;
    }
    return (res);
}

void cps_bind(int addr)
{
    assocsym(addr, acc);
}

void cps_unbind(int n)
{
    int new;
    new = ep;
    while (n > 0) {
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

//------ protect data from GC
void push_protect(int addr)
{
    protect[pp++] = addr;
}

void pop_protect(void)
{
    pp--;
}

//-------read()--------

void gettoken(void)
{
    char c, c1;
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
    c = fgetc(GET_STM(input_stream));
    while ((c == SPACE) || (c == EOL) || (c == TAB))
	c = fgetc(GET_STM(input_stream));

    if (c == ';') {
	while (c != EOL && c != EOF) {
	    c = fgetc(GET_STM(input_stream));
	}
	if (c == EOF) {
	    stok.type = FILEEND;
	    return;
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
	while (((c = fgetc(GET_STM(input_stream))) != EOL) && (pos < BUFSIZE) &&
	       (c != '"'))
	    stok.buf[pos++] = c;

	stok.buf[pos] = NUL;
	stok.type = STRING;
	return;
    case '#':
	c1 = c;
	c = getc(GET_STM(input_stream));
	if (c == '(') {
	    stok.type = VECTOR;
	    return;
	}
	ungetc(c, GET_STM(input_stream));
	c = c1;
	goto etc;
    case '|':
	pos = 0;
	stok.buf[pos++] = c;
	while (((c = fgetc(GET_STM(input_stream))) != EOL) && (pos < BUFSIZE) &&
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
	  etc:
	    pos = 0;
	    stok.buf[pos++] = c;
	    while (((c = fgetc(GET_STM(input_stream))) != EOL) && (pos < BUFSIZE) &&
		   (c != SPACE) && (c != '(') && (c != ')') &&
		   (c != '`') && (c != ',') && (c != '@'))
		stok.buf[pos++] = c;

	    stok.buf[pos] = NUL;
	    stok.ch = c;

	    if (booltoken(stok.buf)) {
		stok.type = BOOLEAN;
		break;
	    }
	    if (chartoken(stok.buf)) {
		stok.type = CHARACTER;
		break;
	    }
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

int booltoken(char buf[])
{
    if (buf[0] == '#' && (buf[1] == 't' || buf[1] == 'f'))
	return (1);
    else
	return (0);
}

int chartoken(char buf[])
{
    if (buf[0] == '#' && buf[1] == '\\')
	return (1);
    else
	return (0);
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
	    else if (c == '.')
		goto dot;
	    else if (c == 'e' || c == 'E')
		goto exp;
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
    if (buf[i] == '+' || buf[i] == '-')
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
    case BOOLEAN:
	if (stok.buf[1] == 't')
	    return (TRUE);
	else
	    return (FAIL);
    case CHARACTER:
	return (makechar(stok.buf));
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
    case VECTOR:
	return (makevec(readlist()));
	break;
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
	fprintf(GET_STM(output_stream),"%d", GET_INT(addr));
	return;
    }
    switch (GET_TAG(addr)) {
    case FLTN:
	fprintf(GET_STM(output_stream),"%g", GET_FLT(addr));
	if (GET_FLT(addr) - (int) GET_FLT(addr) == 0.0)
	    fprintf(GET_STM(output_stream),".0");
	break;
    case STR:
	if (display_flag) {
	    fprintf(GET_STM(output_stream),"%s", GET_NAME(addr));
	} else {
	    fprintf(GET_STM(output_stream),"\"%s\"", GET_NAME(addr));
	}
	break;
    case SYM:
	fprintf(GET_STM(output_stream),"%s", GET_NAME(addr));
	break;
    case BOOL:
	fprintf(GET_STM(output_stream),"%s", GET_NAME(addr));
	break;
    case CHAR:
	fprintf(GET_STM(output_stream),"%s", GET_NAME(addr));
	break;
    case SUBR:
	fprintf(GET_STM(output_stream),"<subr>");
	break;
    case FSUBR:
	fprintf(GET_STM(output_stream),"<fsubr>");
	break;
    case EXPR:
	fprintf(GET_STM(output_stream),"<expr>");
	break;
    case CONT:
	fprintf(GET_STM(output_stream),"<cont>");
	break;
    case PROM:
	fprintf(GET_STM(output_stream),"<promise>");
	break;
    case STM:
	fprintf(GET_STM(output_stream),"<stream> %s",GET_NAME(addr));
	break;
    case LIS:{
	    fprintf(GET_STM(output_stream),"(");
	    printlist(addr);
	    break;
	}
    case VEC:{
	    fprintf(GET_STM(output_stream),"#(");
	    printvec(addr);
	    fprintf(GET_STM(output_stream),")");
	    break;
	}
    default:
	fprintf(GET_STM(output_stream),"<undef>");
	break;
    }
}


void printlist(int addr)
{
    if (IS_NIL(addr))
	fprintf(GET_STM(output_stream),")");
    else if ((!(listp(cdr(addr)))) && (!(nullp(cdr(addr))))) {
	print(car(addr));
	fprintf(GET_STM(output_stream)," . ");
	print(cdr(addr));
	fprintf(GET_STM(output_stream),")");
    } else {
	print(GET_CAR(addr));
	if (!(IS_NIL(GET_CDR(addr))))
	    fprintf(GET_STM(output_stream)," ");
	printlist(GET_CDR(addr));
    }
}

void printvec(int addr)
{
    int n, i;
    n = GET_CDR(addr);
    for (i = 0; i < n; i++) {
	print(GET_VEC_ELT(addr, i));
	if (i < n - 1)
	    fprintf(GET_STM(output_stream)," ");
    }
}



//--------eval---------------
// transfer expr arguments
int transfer_exprargs(int args, int varlist)
{
    if (nullp(args))
	return (NIL);
    else
	return (append(transfer(car(args)),
		       cons(list2
			    (makesym("bind"),
			     list2(makesym("quote"), car(varlist))),
			    transfer_exprargs(cdr(args), cdr(varlist)))));

}

int transfer_exprbody(int addr)
{
    if (nullp(addr))
	return (NIL);
    else
	return (append(transfer(car(addr)), transfer_exprbody(cdr(addr))));
}

int transfer_subrargs(int args)
{
    if (nullp(args))
	return (NIL);
    else {
	return (append(transfer(car(args)),
		       cons(list1(makesym("push")),
			    transfer_subrargs(cdr(args)))));
    }

}

int transfer_fsubrargs(int args)
{
    if (nullp(args))
	return (NIL);
    else {
	return (cons(list2(makesym("quote"), car(args)),
		     cons(list1(makesym("push")),
			  transfer_fsubrargs(cdr(args)))));
    }
}

int transfer_loopargs(int args,int update)
{
    if (nullp(args))
	return (NIL);
    else {
	return (append(transfer(list3(makesym("set!"),car(args),car(update))),
			    transfer_loopargs(cdr(args),cdr(update))));
    }

}


int maltiple_recur_p(int func, int arg)
{
    while (!nullp(arg)) {
	if (eqp(car(car(arg)), func))
	    return (1);
	arg = cdr(arg);
    }
    return (0);
}


int transfer(int addr)
{
    int func, varlist, args, body, loop;
    if (numberp(addr) || stringp(addr) || symbolp(addr)
	|| continuationp(addr) || booleanp(addr) || characterp(addr)
	|| vectorp(addr))
	return (list1(addr));
    else if (listp(addr)) {
	if (numberp(car(addr)))
	    error(ILLEGAL_OBJ_ERR, "transfer", addr);
	else if (subrp(car(addr))) {
        // procedure
	    args = transfer_subrargs(cdr(addr));
	    body =
		list3(makesym("apply-cps"),
		      car(addr),
		      list2(makesym("pop"), makeint(length(cdr(addr)))));
	    return (append(args, list1(body)));
	} else if (fsubrp(car(addr))) {
        //syntax e.g. if cond
	    args = transfer_fsubrargs(cdr(addr));
	    body =
		list3(makesym("apply-cps"),
		      car(addr),
		      list2(makesym("pop"), makeint(length(cdr(addr)))));
	    return (append(args, list1(body)));
	} else if (experp(car(addr))) {
        // function binded in environment
	    func = findsym(car(addr));
	    varlist = car(GET_BIND(func));
	    body = transfer_exprbody(cdr(GET_BIND(func)));
	    args = transfer_exprargs(cdr(addr), varlist);
	    return (append(list1(list2(makesym("set-clos"), car(addr))),
			   append(args, append(body,
					       append(list1
						      (list2
						       (makesym("unbind"),
							makeint(length
								(cdr
								 (addr))))),
						      list1(list1
							    (makesym
							     ("free-clos"))))))));
	} else if (functionp(car(addr))) {
        // functions binded to symbol
	    if (maltiple_recur_p(car(addr), cdr(addr))) {
		return (list1
			(list2
			 (makesym("eval"),
			  list2(makesym("quote"), addr))));
	    } else if (in_recur_p(car(addr))) {
        // in recursion not generate set-clos free-clos
		func = car(addr);
		varlist = car(GET_BIND(GET_BIND(func)));
		body = transfer_exprbody(cdr(GET_BIND(GET_BIND(func))));
		args = transfer_exprargs(cdr(addr), varlist);
		return (append(args, append(body,
					    list1(list2
						  (makesym("unbind"),
						   makeint(length
							   (cdr
							    (addr))))))));
	    } else {
        // normal closure
		func = car(addr);
		varlist = car(GET_BIND(GET_BIND(func)));
		body = transfer_exprbody(cdr(GET_BIND(GET_BIND(func))));
		args = transfer_exprargs(cdr(addr), varlist);
		return (append(list1(list2(makesym("set-clos"), func)),
			       append(args, append(body,
						   append(list1
							  (list2
							   (makesym
							    ("unbind"),
							    makeint(length
								    (cdr
								     (addr))))),
							  list1(list1
								(makesym
								 ("free-clos"))))))));
	    }
	} else if (symbolp(car(addr)) && IS_CONT(findsym(car(addr)))) {
        //e.g. (lambda (c) (set! f c) 0)
	    args = transfer_subrargs(addr);
	    body =
		list3(makesym("apply-cps"),
		      makesym("exec-cont"),
		      list2(makesym("pop"), makeint(length(addr))));
	    return (append(args, list1(body)));
	} else if (symbolp(car(addr)) && car(addr) == continuation_variable) {
        // e.g. (lambda (c) (c 42) 0)
        // execute continuation by exec-cont
        addr = cons(continutation_entity,cdr(addr));
        continuation_variable = NIL;
        continutation_entity = NIL;
	    args = transfer_subrargs(addr);
	    body =
		list3(makesym("apply-cps"),
		      makesym("exec-cont"),
		      list2(makesym("pop"), makeint(length(addr))));
	    return (append(args, list1(body)));
	} else if (lambdap(car(addr))) {
        // lambda expression
	    func = car(addr);
	    varlist = cadr(func);
	    body = transfer_exprbody(cddr(func));
	    args = transfer_exprargs(cdr(addr), varlist);
	    return (append(args, append(body,
					list1
					(list2
					 (makesym("unbind"),
					  makeint(length(cdr(addr))))))));
	} else if (let_loop_p(car(addr))) {
        // named let e.g. (loop (+ n 1))
        varlist = car(GET_CAR(car(addr)));
        body = cdr(GET_CAR(car(addr)));
        args = transfer_loopargs(varlist,cdr(addr));
        loop = transfer_exprbody(body);
	    return (append(args, loop));
	}
    }

    error(ILLEGAL_OBJ_ERR, "transfer", addr);
    return (NIL);
}

// execute continuation
int execute_args(int addr)
{
    if (nullp(addr))
	return (NIL);
    else
	return (cons(execute(car(addr)), execute_args(cdr(addr))));
}

int execute(int addr)
{
    int lam;

    if (numberp(addr) || stringp(addr) || continuationp(addr)
	|| booleanp(addr) || characterp(addr) || vectorp(addr))
	return (addr);
    else if (addr == T || addr == NIL)
	return (addr);
    else if (symbolp(addr)) {
	int res;
	res = findsym(addr);
	if (res != NO)
	    return (res);
	else if (functionp(addr) || subrp(addr) || fsubrp(addr))
	    return (GET_BIND(addr));
	else
	    error(CANT_FIND_ERR, "execute", addr);
    } else if (listp(addr)) {
	if (eqp(car(addr), makesym("quote")))
	    return (cadr(addr));
	else if (subrp(car(addr)) || fsubrp(car(addr))) {
	    int func, args;
	    func = GET_BIND(car(addr));
	    args = execute_args(cdr(addr));
	    return ((GET_SUBR(func)) (args));
	} else if (symbolp(car(addr))
		   && IS_CONT((lam = findsym(car(addr))))) {
	    return (apply_cps(lam, cdr(addr)));
	}
    }
    //dummy to avoid warning
    return (NIL);
}


void print_env(void)
{

    printf("env[");
    print(ep);
    printf("],stack[");
    print(sp_cps);
    printf("]");
}


// eval CPS
int eval_cps(int addr)
{
    int exp, c;
    if (addr != NIL)
	cp = append(transfer(addr), cp);
    while (!nullp(cp)) {
	exp = car(cp);
	cp = cdr(cp);
	acc = execute(exp);
	checkgbc();
	if (step_flag) {
	    print(cp);
	    printf("---cont---\n");
	    print(exp);
	    printf(" in ");
	    print_env();
	    printf(" >> ");
	    c = getc(stdin);
	    if (c == 'q')
		longjmp(buf, 1);
	}
    }
    return (acc);
}

int apply_cps(int func, int args)
{
    int varlist, body, arglist;
    switch (GET_TAG(func)) {
    case SUBR:
	return ((GET_SUBR(func)) (args));
    case FSUBR:
	return ((GET_SUBR(func)) (args));
    case EXPR:
	//(lambda (x) ...)
	varlist = car(GET_BIND(func));
    if(continuationp(car(args))){
    continuation_variable = car(varlist);
    continutation_entity = car(args);
    }
	body = transfer_exprbody(cdr(GET_BIND(func)));
	arglist = transfer_exprargs(args, varlist);
	cp = append(arglist,
		    append(body,
			   append(list1
				  (list2
				   (makesym("unbind"),
				    makeint(length(args)))), cp)));
	//print(cp);
	return (eval_cps(NIL));
    case CONT:
	//continuation
	sp_cps = GET_CAR(func);
	cp = GET_CDR(func);
	return (eval_cps(NIL));
    default:
	error(ILLEGAL_OBJ_ERR, "apply", func);
    }
    return (0);
}

int eval(int addr)
{
    int res, lam;

    if (step_flag) {
	int c;
	print(addr);
	printf(" in ");
	print_env();
	printf(" >> ");
	c = getc(stdin);
	if (c == 'q')
	    longjmp(buf, 1);
    }
    if (atomp(addr)) {
	if (numberp(addr) || stringp(addr) || characterp(addr)
	    || booleanp(addr))
	    return (addr);
	if (symbolp(addr)) {
	    res = findsym(addr);
	    if (res != NO)
		return (res);
	    else if (functionp(addr) || subrp(addr) || fsubrp(addr))
		return (GET_BIND(addr));
	    error(CANT_FIND_ERR, "eval", addr);
	}
    } else if (listp(addr)) {
	if (numberp(car(addr)))
	    error(ARG_SYM_ERR, "eval", addr);
	else if (lambdap(car(addr)))
	    return (apply(eval(car(addr)), cdr(addr)));
	else if (subrp(car(addr)))
	    return (apply(GET_BIND(car(addr)), evlis(cdr(addr))));
	else if (fsubrp(car(addr)))
	    return (apply(GET_BIND(car(addr)), cdr(addr)));
	else if (symbolp(car(addr)) && IS_EXPR((lam = findsym(car(addr)))))
	    return (apply(lam, evlis(cdr(addr))));
	else if (functionp(car(addr))) {
	return (apply(GET_BIND(car(addr)), evlis(cdr(addr))));
	}
    }
    error(CANT_FIND_ERR, "eval", addr);
    return (0);
}

int apply(int func, int args)
{
    int varlist, body, res;

    res = NIL;
    switch (GET_TAG(func)) {
    case SUBR:
	return ((GET_SUBR(func)) (args));
    case FSUBR:
	return ((GET_SUBR(func)) (args));
    case EXPR:{
	    varlist = car(GET_BIND(func));
	    body = cdr(GET_BIND(func));
	    bindarg(varlist, args);
	    while (!(IS_NIL(body))) {
		res = eval(car(body));
		body = cdr(body);
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

    case DIV_ZERO_ERR:{
	    printf("%s division by zero ", fun);
	    print(arg);
	    break;
	}

     case ARITY_ERR:{
	    printf("%s arity mismatch ", fun);
	    print(arg);
	    break;
	}

    case ARG_SYM_ERR:{
	    printf("%s require symbol but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_BOOL_ERR:{
	    printf("%s require bool but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_INT_ERR:{
	    printf("%s require integer but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_VEC_ERR:{
	    printf("%s require vector but got ", fun);
	    print(arg);
	    break;
	}

    case ARG_PROM_ERR:{
	    printf("%s require promise but got ", fun);
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

    case ARG_CHAR_ERR:{
	    printf("%s require character but got ", fun);
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
    input_stream = STDIN;
    output_stream = STDOUT;
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
    case INTEGER_TEST:
	if (integerp(arg))
	    return;
	else
	    error(ARG_INT_ERR, fun, arg);
    case STRING_TEST:
	if (stringp(arg))
	    return;
	else
	    error(ARG_STR_ERR, fun, arg);
    case CHAR_TEST:
	if (characterp(arg))
	    return;
	else
	    error(ARG_CHAR_ERR, fun, arg);
    case LIST_TEST:
	if (listp(arg))
	    return;
	else
	    error(ARG_LIS_ERR, fun, arg);
    case BOOL_TEST:
	if (booleanp(arg))
	    return;
	else
	    error(ARG_BOOL_ERR, fun, arg);
    case VECTOR_TEST:
	if (vectorp(arg))
	    return;
	else
	    error(ARG_VEC_ERR, fun, arg);
    case PROMISE_TEST:
	if ((arg > 0 || arg < HEAPSIZE) && IS_PROM(arg))
	    return;
	else
	    error(ARG_PROM_ERR, fun, arg);
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
    case DIVZERO_TEST:
	if (!zerop(arg))
	    return;
	else
	    error(DIV_ZERO_ERR, fun, arg);

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
    while (!nullp(arg)) {
	if (!symbolp(car(arg)))
	    return (0);
	arg = cdr(arg);
    }
    return (1);
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
    SET_CDR(val, copy(ep));
    SET_BIND(sym, val);
}

void initsubr(void)
{
    defsubr("+", f_plus);
    defsubr("-", f_difference);
    defsubr("*", f_times);
    defsubr("/", f_division);
    defsubr("quotient", f_quotient);
    defsubr("max", f_max);
    defsubr("min", f_min);
    defsubr("abs", f_abs);
    defsubr("remainder", f_remainder);
    defsubr("modulo", f_modulo);
    defsubr("expt", f_expt);
    defsubr("sqrt", f_sqrt);
    defsubr("exp", f_exp);
    defsubr("log", f_log);
    defsubr("sin", f_sin);
    defsubr("cos", f_cos);
    defsubr("tan", f_tan);
    defsubr("asin", f_asin);
    defsubr("acos", f_acos);
    defsubr("atan", f_atan);
    defsubr("gcd", f_gcd);
    defsubr("lcm", f_lcm);
    defsubr("floor", f_floor);
    defsubr("ceiling", f_ceiling);
    defsubr("truncate", f_truncate);
    defsubr("round", f_round);
    defsubr("exit", f_exit);
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
    defsubr("list-tail", f_list_tail);
    defsubr("list-ref", f_list_ref);
    defsubr("length", f_length);
    defsubr("append", f_append);
    defsubr("append!", f_nconc);
    defsubr("set-car!", f_rplaca);
    defsubr("set-cdr!", f_rplacd);
    defsubr("map", f_map);
    defsubr("for-each", f_for_each);
    defsubr("=", f_numeqp);
    defsubr("eq?", f_eq);
    defsubr("eqv?", f_eqv);
    defsubr("equal?", f_equal);
    defsubr("null?", f_nullp);
    defsubr("atom?", f_atomp);
    defsubr("gbc", f_gbc);
    defsubr("read", f_read);
    defsubr("read-char", f_read_char);
    defsubr("eval", f_eval);
    defsubr("eval-cps", f_eval_cps);
    defsubr("apply", f_apply);
    defsubr("apply-cps", f_apply_cps);
    defsubr("display", f_display);
    defsubr("write", f_write);
    defsubr("newline", f_newline);
    defsubr("gensym", f_gensym);
    defsubr("step", f_step);
    defsubr("putprop", f_putprop);
    defsubr("get", f_get);
    defsubr(">", f_greaterp);
    defsubr("<", f_lessp);
    defsubr(">=", f_eqgreaterp);
    defsubr("<=", f_eqlessp);
    defsubr("zero?", f_zerop);
    defsubr("positive?", f_positivep);
    defsubr("negative?", f_negativep);
    defsubr("odd?", f_oddp);
    defsubr("even?", f_evenp);
    defsubr("exact?", f_exactp);
    defsubr("inexact", f_inexactp);
    defsubr("number?", f_numberp);
    defsubr("rational?", f_rationalp);
    defsubr("symbol?", f_symbolp);
    defsubr("list?", f_listp);
    defsubr("vector?", f_vectorp);
    defsubr("pair?", f_pairp);
    defsubr("boolean?", f_booleanp);
    defsubr("assoc", f_assoc);
    defsubr("member", f_member);
    defsubr("memq", f_memq);
    defsubr("memv", f_memv);
    defsubr("set", f_set);
    defsubr("not", f_not);
    defsubr("load", f_load);
    defsubr("edwin", f_edwin);
    defsubr("functionp", f_functionp);
    defsubr("procedure?", f_procedurep);
    defsubr("promise?", f_promisep);
    defsubr("call-with-current-continuation", f_call_cc);
    defsubr("call/cc", f_call_cc);
    defsubr("push", f_push);
    defsubr("pop", f_pop);
    defsubr("bind", f_bind);
    defsubr("unbind", f_unbind);
    defsubr("set-clos", f_set_clos);
    defsubr("free-clos", f_free_clos);
    defsubr("transfer", f_transfer);
    defsubr("exec-cont", f_exec_cont);
    defsubr("environment", f_environment);
    defsubr("analyze", f_analyze);
    defsubr("vector", f_vector);
    defsubr("vector-length", f_vector_length);
    defsubr("vector-ref", f_vector_ref);
    defsubr("symbol->string", f_symbol_to_string);
    defsubr("string->symbol", f_string_to_symbol);
    defsubr("char?", f_charp);
    defsubr("char=?", f_char_eqp);
    defsubr("char<?", f_char_lessp);
    defsubr("char>?", f_char_greaterp);
    defsubr("char<=?", f_char_eqlessp);
    defsubr("char>=?", f_char_eqgreaterp);
    defsubr("char-ci=?", f_char_ci_eqp);
    defsubr("char-ci<?", f_char_ci_lessp);
    defsubr("char-ci>?", f_char_ci_greaterp);
    defsubr("char-ci<=?", f_char_ci_eqlessp);
    defsubr("char-ci>=?", f_char_ci_eqgreaterp);
    defsubr("char-alphabetic?", f_char_alphabetic_p);
    defsubr("char-numeric?", f_char_numeric_p);
    defsubr("char-whitespace?", f_char_whitespace_p);
    defsubr("char-upper-case?", f_char_upper_case_p);
    defsubr("char-lower-case?", f_char_lower_case_p);
    defsubr("string?", f_stringp);
    defsubr("make-string", f_make_string);
    defsubr("string=?", f_str_eqp);
    defsubr("string<?", f_str_lessp);
    defsubr("string>?", f_str_greaterp);
    defsubr("string<=?", f_str_eqlessp);
    defsubr("string>=?", f_str_eqgreaterp);
    defsubr("string-ci=?", f_str_ci_eqp);
    defsubr("string-ci<?", f_str_ci_lessp);
    defsubr("string-ci>?", f_str_ci_greaterp);
    defsubr("string-ci<=?", f_str_ci_eqlessp);
    defsubr("string-ci>=?", f_str_ci_eqgreaterp);
    defsubr("string-length", f_string_length);
    defsubr("string-ref", f_string_ref);
    defsubr("string-set!", f_string_set);
    defsubr("string-append", f_string_append);
    defsubr("string->list", f_string_to_list);
    defsubr("list->string", f_list_to_string);
    defsubr("string-copy", f_string_copy);
    defsubr("string-fill!", f_string_fill);
    defsubr("substring", f_substring);
    defsubr("input-port?", f_input_port_p);
    defsubr("output-port?", f_output_port_p);
    defsubr("current-input-port", f_current_input_port);
    defsubr("current-output-port", f_current_output_port);
    defsubr("open-input-file", f_open_input_file);
    defsubr("open-output-file", f_open_output_file);
    defsubr("close-input-port", f_close_input_port);
    defsubr("close-output-port", f_close_output_port);
    defsubr("call-with-input-file", f_call_with_input_file);
    defsubr("call-with-output-file", f_call_with_output_file);
    defsubr("exact->inexact", f_exact_to_inexact);
    defsubr("inexact->exact", f_inexact_to_exact);
    defsubr("number->string", f_number_to_string);
    defsubr("string->number", f_string_to_number);


    deffsubr("quote", f_quote);
    deffsubr("set!", f_setq);
    deffsubr("define", f_define);
    deffsubr("lambda", f_lambda);
    deffsubr("begin", f_begin);
    deffsubr("if", f_if);
    deffsubr("cond", f_cond);
    deffsubr("case", f_case);
    deffsubr("and", f_and);
    deffsubr("or", f_or);
    deffsubr("let", f_let);
    deffsubr("let*", f_let_star);
    deffsubr("letrec", f_letrec);
    deffsubr("do", f_do);
    deffsubr("delay", f_delay);
    defsubr("force", f_force);

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
	    return (makeflt((double) GET_INT(x) / GET_FLT(y)));
    } else if (floatp(x)) {
	if (integerp(y))
	    return (makeflt(GET_FLT(x) / (double) GET_INT(y)));
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

int f_difference(int arglist)
{
    int arg1, arg2;
    checkarg(NUMLIST_TEST, "difference", arglist);
    if (length(arglist) == 2) {
	arg1 = car(arglist);
	arg2 = cadr(arglist);
	return (difference(arg1, arg2));
    } else if (length(arglist) == 1) {
	arg1 = car(arglist);
	return (times(arg1, makeint(-1)));
    } else if (length(arglist) > 2) {
    arg1 = car(arglist);
    arg2 = f_plus(cdr(arglist));
    return (difference(arg1, arg2));
    }
    //dummy
    return (TRUE);
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


int f_division(int arglist)
{
    int arg1, arg2;

    checkarg(NUMLIST_TEST, "/", arglist);
    if (length(arglist) == 2){
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    checkarg(DIVZERO_TEST,"/",arg2);
    return (quotient(arg1, arg2));
    } else if(length(arglist) == 1){
    arg1 = makeint(1);
    arg2 = car(arglist);
    return (quotient(arg1, arg2));    
    } else if (length(arglist) > 2){
    arg1 = car(arglist);
    arg2 = f_times(cdr(arglist));
    checkarg(DIVZERO_TEST,"/",arg2);
    return (quotient(arg1, arg2));     
    }
    // dummy
    return(TRUE);
}

int f_quotient(int arglist)
{
    int arg1,arg2;
    checkarg(INTLIST_TEST,"quotient",arglist);
    checkarg(DIVZERO_TEST,"quotient",cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return(makeint(GET_INT(arg1) / GET_INT(arg2)));
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

int f_abs(int arglist)
{
    int arg1, res;
    checkarg(NUMBER_TEST, "abs", car(arglist));
    arg1 = car(arglist);
    res = NIL;
    if (integerp(arg1))
	res = makeint(abs(GET_INT(arg1)));
    else if (floatp(arg1))
	res = makeflt(fabs(GET_FLT(arg1)));

    return (res);
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

int modulo(int x, int y)
{
    int r = x % y;
    if ((r > 0 && y < 0) || (r < 0 && y > 0)) {
	r += y;
    }
    return r;
}

int f_modulo(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "modulo", arglist);
    checkarg(INTLIST_TEST, "modulo", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (makeint(modulo(GET_INT(arg1), GET_INT(arg2))));
}

int f_expt(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "expt", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if (integerp(arg1) && integerp(arg2))
	return (makeint(power(GET_INT(arg1), GET_INT(arg2))));
    else {
	arg1 = exact_to_inexact(arg1);
	arg2 = exact_to_inexact(arg2);
	return (makeflt(pow(GET_FLT(arg1), GET_FLT(arg2))));
    }
}

int f_sqrt(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "sqrt", arglist);
    checkarg(NUMBER_TEST, "sqrt", car(arglist));
    arg1 = car(arglist);
    res = 0;
    if (integerp(arg1))
	res = sqrt((double) GET_INT(arg1));
    else if (floatp(arg1))
	res = sqrt(GET_FLT(arg1));

    return (makeflt(res));
}



int f_exp(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "exp", arglist);
    checkarg(NUMBER_TEST, "exp", car(arglist));
    arg1 = exact_to_inexact(car(arglist));
    res = exp(GET_FLT(arg1));
    return (makeflt(res));
}



int f_log(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "log", arglist);
    checkarg(NUMBER_TEST, "log", car(arglist));
    arg1 = exact_to_inexact(car(arglist));
    res = log(GET_FLT(arg1));
    return (makeflt(res));
}


int f_sin(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "sin", arglist);
    checkarg(NUMBER_TEST, "sin", car(arglist));
    arg1 = exact_to_inexact(car(arglist));
    res = sin(GET_FLT(arg1));
    return (makeflt(res));
}


int f_cos(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "cos", arglist);
    checkarg(NUMBER_TEST, "cos", car(arglist));
    arg1 = exact_to_inexact(car(arglist));
    res = cos(GET_FLT(arg1));
    return (makeflt(res));
}


int f_tan(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "tan", arglist);
    checkarg(NUMBER_TEST, "tan", car(arglist));
    arg1 = exact_to_inexact(car(arglist));
    res = tan(GET_FLT(arg1));
    return (makeflt(res));
}

int f_asin(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "asin", arglist);
    checkarg(NUMBER_TEST, "asin", car(arglist));
    arg1 = exact_to_inexact(car(arglist));
    res = asin(GET_FLT(arg1));
    return (makeflt(res));
}

int f_acos(int arglist)
{
    int arg1;
    double res;
    checkarg(LEN1_TEST, "acos", arglist);
    checkarg(NUMBER_TEST, "acos", car(arglist));
    arg1 = exact_to_inexact(car(arglist));
    res = acos(GET_FLT(arg1));
    return (makeflt(res));
}

int f_atan(int arglist)
{
    int arg1;
    double res;
    if (length(arglist) == 1) {
	checkarg(NUMBER_TEST, "atan", car(arglist));
	arg1 = exact_to_inexact(car(arglist));
	res = atan(GET_FLT(arg1));
    } else if (length(arglist) == 2) {
	checkarg(NUMBER_TEST, "atan", car(arglist));
	checkarg(NUMBER_TEST, "atan", cadr(arglist));
	int arg2;
	arg1 = exact_to_inexact(car(arglist));
	arg2 = exact_to_inexact(cadr(arglist));
	res = atan2(GET_FLT(arg1), GET_FLT(arg2));
    } else {
	error(ARITY_ERR, "atan", arglist);
    }
    return (makeflt(res));
}

int gcd(int a, int b)
{
    a = abs(a);
    b = abs(b);
    while (b != 0) {
	int t = b;
	b = a % b;
	a = t;
    }
    return a;
}

int lcm(int a, int b)
{
    if (a == 0 || b == 0)
	return 0;
    return abs(a * b) / gcd(a, b);
}

int f_gcd(int arglist)
{

    checkarg(INTLIST_TEST, "gcd", arglist);
    if (nullp(cdr(arglist)))
	return (car(arglist));
    else {
	int a, b, x;
	a = GET_INT(car(arglist));
	b = GET_INT(f_gcd(cdr(arglist)));
	x = gcd(a, b);
	return (makeint(x));
    }

}

int f_lcm(int arglist)
{
    checkarg(INTLIST_TEST, "lcm", arglist);
    if (nullp(cdr(arglist)))
	return (car(arglist));
    else {
	int a, b, x;
	a = GET_INT(car(arglist));
	b = GET_INT(f_lcm(cdr(arglist)));
	x = lcm(a, b);
	return (makeint(x));
    }

}

int f_floor(int arglist)
{
    int arg1;
    double res;
    checkarg(NUMBER_TEST,"floor",car(arglist));
    arg1 = car(arglist);
    if(integerp(arg1))
    return(arg1);
    
    res = floor(GET_FLT(arg1));
    return(makeflt(res));
    
}


int f_ceiling(int arglist)
{
    int arg1;
    double res;
    checkarg(NUMBER_TEST,"ceiling",car(arglist));
    arg1 = car(arglist);
    if(integerp(arg1))
    return(arg1);
    
    res = ceil(GET_FLT(arg1));
    return(makeflt(res));
    
}


int f_truncate(int arglist)
{
    int arg1;
    double res;
    checkarg(NUMBER_TEST,"truncate",car(arglist));
    arg1 = car(arglist);
    if(integerp(arg1))
    return(arg1);
    
    res = trunc(GET_FLT(arg1));
    return(makeflt(res));
    
}


int f_round(int arglist)
{
    int arg1;
    double res;
    checkarg(NUMBER_TEST,"round",car(arglist));
    arg1 = car(arglist);
    if(integerp(arg1))
    return(arg1);
    
    res = round(GET_FLT(arg1));
    return(makeflt(res));
    
}


int f_exit(int arglist)
{
    int addr;

    checkarg(LEN0_TEST, "exit", arglist);
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
    return (TRUE);
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

int f_caar(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "caar", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "caar", arg1);
    return (caar(arg1));
}

int f_cadr(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "cadr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "cadr", arg1);
    return (cadr(arg1));
}

int f_cdar(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "cdar", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "cdar", arg1);
    return (cdar(arg1));
}


int f_cddr(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "cddr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "cddr", arg1);
    return (cddr(arg1));
}

int f_caddr(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "caddr", arglist);
    arg1 = car(arglist);
    if (atomp(arg1))
	error(ARG_LIS_ERR, "caddr", arg1);
    return (caddr(arg1));
}

int f_cadddr(int arglist)
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


int f_numeqp(int arglist)
{
    int arg1, arg2;

    checkarg(NUMLIST_TEST, "=", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (nullp(arg2))
    return(TRUE);
    else if (numeqp(arg1, arg2) && f_numeqp(cdr(arglist)))
	return (TRUE);
    else
	return (FAIL);
}


int f_eq(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "eq", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if (eqp(arg1, arg2))
	return (TRUE);
    else
	return (FAIL);
}


int f_eqv(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "eqv", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if (eqvp(arg1, arg2))
	return (TRUE);
    else
	return (FAIL);
}


int f_equal(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "equal", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    if (equalp(arg1, arg2))
	return (TRUE);
    else
	return (FAIL);
}

int f_nullp(int arglist)
{
    int arg;

    checkarg(LEN1_TEST, "null", arglist);
    arg = car(arglist);
    if (nullp(arg))
	return (TRUE);
    else
	return (FAIL);
}

int f_atomp(int arglist)
{
    int arg;

    checkarg(LEN1_TEST, "atom?", arglist);
    arg = car(arglist);
    if (atomp(arg))
	return (TRUE);
    else
	return (FAIL);
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

int f_list_tail(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "list-tail", arglist);
    checkarg(LIST_TEST, "list-tail", car(arglist));
    checkarg(INTEGER_TEST, "list-tail", cadr(arglist));

    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (list_tail(arg1, GET_INT(arg2)));
}

int f_list_ref(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "list-ref", arglist);
    checkarg(LIST_TEST, "list-ref", car(arglist));
    checkarg(INTEGER_TEST, "list-ref", cadr(arglist));

    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (car(list_tail(arg1, GET_INT(arg2))));
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


int f_memq(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "memq", arglist);
    checkarg(LIST_TEST, "memq", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (memq(arg1, arg2));
}


int f_memv(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "memv", arglist);
    checkarg(LIST_TEST, "memv", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (memv(arg1, arg2));
}



int f_append(int arglist)
{
    if (nullp(arglist))
	return (NIL);
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
    return (UNDEF);
}

int f_rplacd(int arglist)
{
    int arg1, arg2;

    checkarg(LEN2_TEST, "rplacd", arglist);
    checkarg(LIST_TEST, "rplacd", car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    SET_CDR(arg1, arg2);
    return (UNDEF);
}


int f_symbolp(int arglist)
{
    checkarg(LEN1_TEST, "symbol?", arglist);
    if (symbolp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}

int f_numberp(int arglist)
{
    checkarg(LEN1_TEST, "number?", arglist);
    if (numberp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}

int f_rationalp(int arglist)
{
    checkarg(LEN1_TEST, "rational?", arglist);
    if (rationalp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}

int f_listp(int arglist)
{
    if (listp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}


int f_vectorp(int arglist)
{
    if (vectorp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}



int f_pairp(int arglist)
{
    if (pairp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}


int f_booleanp(int arglist)
{
    checkarg(LEN1_TEST, "boolean?", arglist);
    if (booleanp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}

int f_zerop(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "zerop", arglist);
    checkarg(NUMLIST_TEST, "zerop", arglist);
    arg1 = car(arglist);

    if (zerop(arg1))
	return (TRUE);
    else
	return (FAIL);
}



int f_positivep(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "positive?", arglist);
    checkarg(NUMLIST_TEST, "positive?", arglist);
    arg1 = car(arglist);

    if (greaterp(arg1, makeint(0)))
	return (TRUE);
    else
	return (FAIL);
}


int f_negativep(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "negative?", arglist);
    checkarg(NUMLIST_TEST, "negative?", arglist);
    arg1 = car(arglist);

    if (lessp(arg1, makeint(0)))
	return (TRUE);
    else
	return (FAIL);
}


int f_oddp(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "odd?", arglist);
    checkarg(NUMLIST_TEST, "odd?", arglist);
    arg1 = car(arglist);

    if (integerp(arg1) && GET_INT(arg1) % 2 == 1)
	return (TRUE);
    else
	return (FAIL);
}


int f_evenp(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "even?", arglist);
    checkarg(NUMLIST_TEST, "even?", arglist);
    arg1 = car(arglist);

    if (integerp(arg1) && GET_INT(arg1) % 2 == 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_exactp(int arglist)
{
    checkarg(LEN1_TEST, "exact?", arglist);
    checkarg(NUMBER_TEST, "exact?", car(arglist));
    if (integerp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}

int f_inexactp(int arglist)
{
    checkarg(LEN1_TEST, "inexact?", arglist);
    checkarg(NUMBER_TEST, "inexact?", car(arglist));
    if (integerp(car(arglist)))
	return (FAIL);
    else
	return (TRUE);
}


int f_lessp(int arglist)
{
    int arg1, arg2;

    checkarg(NUMLIST_TEST, "<", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if(nullp(arg2))
        return(TRUE);
    else if (lessp(arg1, arg2) && f_lessp(cdr(arglist)) == TRUE)
	return (TRUE);
    else
	return (FAIL);
}


int f_eqlessp(int arglist)
{
    int arg1, arg2;

    checkarg(NUMLIST_TEST, "<=", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (nullp(arg2))
    return (TRUE);
    else if ((lessp(arg1, arg2) || numeqp(arg1, arg2)) && f_eqlessp(cdr(arglist)))
	return (TRUE);
    else
	return (FAIL);
}


int f_greaterp(int arglist)
{
    int arg1, arg2;

    checkarg(NUMLIST_TEST, ">", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (nullp(arg2))
    return(TRUE);
    else if (greaterp(arg1, arg2) && f_greaterp(cdr(arglist)))
	return (TRUE);
    else
	return (FAIL);
}


int f_eqgreaterp(int arglist)
{
    int arg1, arg2;

    checkarg(NUMLIST_TEST, ">=", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (nullp(arg2))
    return(TRUE);
    else if ((greaterp(arg1, arg2) || numeqp(arg1, arg2)) && f_eqgreaterp(cdr(arglist)))
	return (TRUE);
    else
	return (FAIL);
}



int f_gbc(int arglist)
{

    if (car(arglist) == TRUE)
	gbc_flag = 1;
    else if (car(arglist) == FAIL)
	gbc_flag = 0;
    else if (car(arglist) == makeint(1)) {
	printf("execute gc!\n");
	gbc();
    }
    return (T);
}

int f_read(int arglist)
{   
    int save,res;
    if(length(arglist) == 0){
    return (read());
    } else if(length(arglist) == 1){
        save = input_stream;
        input_stream = car(arglist);
        res = read();
        input_stream = save;
        return(res);
    } else 
        error(ARITY_ERR,"read",arglist);
    //dummy
    return(TRUE);
}

int f_read_char(int arglist)
{   
    int save;
    char c, str[5];
    memset(str, 0, 5);
    str[0] = '#';
    str[1] = '\\';
    str[3] = 0;
    if(length(arglist) == 0){
    c = fgetc(GET_STM(input_stream));
    str[2] = c;
    return (makechar(str));
    } else if(length(arglist) == 1){
        save = input_stream;
        input_stream = car(arglist);
        c = fgetc(GET_STM(input_stream));
        str[2] = c;
        res = makechar(str);
        input_stream = save;
        return(res);
    } else 
        error(ARITY_ERR,"read-char",arglist);
    //dummy
    return(TRUE);
}

int f_display(int arglist)
{
    int arg1,arg2,save;
    if(length(arglist) == 1){
    arg1 = car(arglist);
    display_flag = 1;
    print(arg1);
    display_flag = 0;
    } else if(length(arglist) == 2){
        arg1 = car(arglist);
        arg2 = cadr(arglist);
        save = output_stream;
        output_stream = arg2;
        display_flag = 1;
        print(arg1);
        display_flag = 0;
        output_stream = save;
    } else 
        error(ARITY_ERR,"display",arglist);
    return (TRUE);
}


int f_write(int arglist)
{
    int arg1,arg2,save;
    if(length(arglist) == 1){
    arg1 = car(arglist);
    display_flag = 1;
    print(arg1);
    display_flag = 0;
    } else if(length(arglist) == 2){
        arg1 = car(arglist);
        arg2 = cadr(arglist);
        save = output_stream;
        output_stream = arg2;
        print(arg1);
        output_stream = save;
    } else 
        error(ARITY_ERR,"write",arglist);
    return (TRUE);
}


int f_newline(int arglist)
{   
    if(length(arglist) == 0){
    printf("\n");
    } else if(length(arglist) == 1){
       fprintf(GET_STM(car(arglist)),"\n"); 
    }
    return (TRUE);
}


int f_gensym(int arglist)
{
    char gsym[SYMSIZE];
    checkarg(LEN0_TEST, "gensym", arglist);
    sprintf(gsym, "G%05d", gennum);
    gennum++;
    return (makesym(gsym));
}

int f_step(int arglist)
{
    int arg1;

    checkarg(LEN1_TEST, "step", arglist);
    arg1 = car(arglist);
    if (arg1 == TRUE) {
	step_flag = 1;
    } else if (arg1 == FAIL)
	step_flag = 0;
    else
	error(ILLEGAL_OBJ_ERR, "step", arg1);

    return (TRUE);
}


int f_putprop(int arglist)
{
    int arg1, arg2, arg3, plist, res;
    checkarg(LEN3_TEST, "putprop", arglist);
    checkarg(SYMBOL_TEST, "putprop", car(arglist));
    checkarg(SYMBOL_TEST, "putprop", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = caddr(arglist);
    plist = GET_CDR(arg1);
    res = assoc(arg2, plist);
    if (res == NO)
	SET_CDR(arg1, cons(cons(arg2, arg3), plist));
    else
	SET_CDR(res, arg3);
    return (T);
}

int f_defprop(int arglist)
{
    int arg1, arg2, arg3;
    checkarg(LEN3_TEST, "defprop", arglist);
    checkarg(SYMBOL_TEST, "defprop", car(arglist));
    checkarg(SYMBOL_TEST, "defprop", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = caddr(arglist);
    SET_CDR(arg1, cons(cons(arg2, arg3), NIL));
    return (T);
}


int f_get(int arglist)
{
    int arg1, arg2, res;
    checkarg(LEN2_TEST, "get", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    res = assoc(arg2, GET_CDR(arg1));
    if (res == NO)
	return (NIL);

    return (cdr(res));
}

int f_eval_cps(int arglist)
{
    checkarg(LEN1_TEST, "eval", arglist);
    return (eval_cps(car(arglist)));
}


int f_eval(int arglist)
{
    checkarg(LEN1_TEST, "eval", arglist);
    return (eval(car(arglist)));
}

int f_apply_cps(int arglist)
{
    checkarg(LEN2_TEST, "apply_cps", arglist);
    checkarg(LIST_TEST, "apply_cps", cadr(arglist));
    int arg1, arg2;

    arg1 = car(arglist);
    arg2 = cadr(arglist);

    return (apply_cps(arg1, arg2));
}

int f_apply(int arglist)
{
    checkarg(LEN2_TEST, "apply", arglist);
    checkarg(LIST_TEST, "apply", cadr(arglist));
    int arg1, arg2;

    arg1 = car(arglist);
    arg2 = cadr(arglist);

    return (apply(arg1, arg2));
}


int f_map(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "map", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (map(arg2, arg1));
}


int f_for_each(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "for-each", arglist);
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    for_each(arg2, arg1);
    return(arg2);
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
    if (arg1 == FAIL)
	return (TRUE);
    else
	return (FAIL);
}

int f_analyze(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    print(arg1);
    printf("\nBIND ");
    print(GET_BIND(arg1));
    printf("\nCAR ");
    print(GET_CAR(arg1));
    printf("\nCDR ");
    print(GET_CDR(arg1));
    printf("\n");

    return (TRUE);
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
    if (functionp(arg1) || subrp(arg1) || fsubrp(arg1))
	error(ILLEGAL_OBJ_ERR, "set!", arg1);
    arg2 = eval(cadr(arglist));
    bindsym(arg1, arg2);
    return (TRUE);
}


void bindfunc1(int sym, int type, int addr)
{
    int val;

    val = freshcell();
    SET_TAG(val, type);
    SET_BIND(val, addr);
    SET_CDR(val, 0);
    SET_BIND(sym, val);
}


int f_define(int arglist)
{
    int arg1, arg2;

    arg1 = NIL;
    if (listp(car(arglist))) {
	arg1 = caar(arglist);	// function name
	arg2 = cons(cdar(arglist), cdr(arglist));	// argument + body
	bindfunc1(arg1, EXPR, arg2);
    } else if (symbolp(car(arglist)) && lambdap(cadr(arglist))) {
	checkarg(LIST_TEST, "define", cadr(arglist));
	arg1 = car(arglist);	// function name
	arg2 = cadr(arglist);	//lambda exp
	SET_BIND(arg1, eval(arg2));
    } else if (symbolp(car(arglist))) {
	arg1 = car(arglist);	//variable name
	arg2 = eval(cadr(arglist));	//value
	bindsym(arg1, arg2);
    }
    return (arg1);
}


int f_lambda(int arglist)
{

    checkarg(LIST_TEST, "lambda", car(arglist));
    checkarg(LIST_TEST, "lambda", cdr(arglist));
    return (makefunc(arglist));
}

int f_if(int arglist)
{
    int arg1, arg2, arg3, res;
    if (length(arglist) == 3) {
	arg1 = car(arglist);
	arg2 = cadr(arglist);
	arg3 = car(cdr(cdr(arglist)));

	cp1 = cp;
	cp = NIL;
	res = eval_cps(arg1);
	cp = cp1;
	if (res != FAIL)
	    return (eval_cps(arg2));
	else
	    return (eval_cps(arg3));
    } else if (length(arglist) == 2) {
	arg1 = car(arglist);
	arg2 = cadr(arglist);
	arg3 = car(cdr(cdr(arglist)));

	cp1 = cp;
	cp = NIL;
	res = eval_cps(arg1);
	cp = cp1;
	if (res != FAIL)
	    return (eval_cps(arg2));
	else
	    return (UNDEF);
    }
    error(ARITY_ERR, "if", arglist);
    return (FAIL);
}

int f_cond(int arglist)
{
    int arg1, arg2, arg3, res;

    if (nullp(arglist))
	return (NIL);


    arg1 = car(arglist);
    checkarg(LIST_TEST, "cond", arg1);
    arg2 = car(arg1);
    arg3 = cdr(arg1);
    if (eqp(arg2, makesym("else"))) {
	res = TRUE;
    } else {
	cp1 = cp;
	cp = NIL;
	res = eval_cps(arg2);
	cp = cp1;
    }

    if (res != FAIL)
	return (f_begin(arg3));
    else
	return (f_cond(cdr(arglist)));
}

int f_case(int arglist)
{
    int arg1, arg2, test;

    checkarg(LIST_TEST, "case", cadr(arglist));
    arg1 = eval(car(arglist));
    arg2 = cdr(arglist);
    while (!nullp(arg2)) {
	test = car(arg2);
	if (memv(arg1, car(test)) != FAIL)
	    return (f_begin(cdr(test)));
	else if (eqp(car(test), makesym("else")))
	    return (f_begin(cdr(test)));
	arg2 = cdr(arg2);
    }
    return (FAIL);
}


int f_begin(int arglist)
{
    arglist = reverse(arglist);
    while (!(nullp(arglist))) {
	cp = append(transfer(car(arglist)), cp);
	arglist = cdr(arglist);
    }
    return (eval_cps(NIL));
}


int f_and(int arglist)
{
    int res;

    res = NIL;
    while (!nullp(arglist)) {
	res = eval_cps(car(arglist));
	if (res == FAIL)
	    return (FAIL);
	arglist = cdr(arglist);
    }
    return (res);
}

int f_or(int arglist)
{
    int res;

    while (!nullp(arglist)) {
	res = eval_cps(car(arglist));
	if (res != FAIL)
	    return (res);
	arglist = cdr(arglist);
    }
    return (FAIL);
}

int f_let(int arglist)
{
    int arg1, arg2, arg3, vars, args, var, val, res, save, save1;
    if (!symbolp(car(arglist))) {
	//normal let syntax
	checkarg(LIST_TEST, "let", car(arglist));
	arg1 = car(arglist); //var list
	arg2 = cdr(arglist); //body
	vars = NIL;

	save = ep;
	save1 = pp;
	while (!nullp(arg1)) {
	    var = caar(arg1);
	    val = eval_cps(cadar(arg1));
	    push_protect(val);
	    arg1 = cdr(arg1);
        vars = cons(cons(var, val), vars);
	}
	ep = save;
	while (!nullp(vars)) {
	    var = caar(vars);
	    val = cdar(vars);
	    vars = cdr(vars);
	    assocsym(var, val);
	}
	res = f_begin(arg2);
	ep = save;
	pp = save1;
	return (res);
    } else {
	// case of let named letlsyntax
	checkarg(LIST_TEST, "let", cadr(arglist));
    arg1 = car(arglist);  //name symbol
	arg2 = cadr(arglist); //var list
	arg3 = cddr(arglist); //body
    vars = NIL;
    args = NIL;
    save = ep;
	save1 = pp;
	while (!nullp(arg2)) {
	    var = caar(arg2);
	    val = eval_cps(cadar(arg2));
	    push_protect(val);
	    arg2 = cdr(arg2);
	    vars = cons(cons(var, val), vars);
        args = cons(var,args);
	}
    SET_CAR(arg1,cons(args,arg3)); // save body to name symbol
	ep = save;
	while (!nullp(vars)) {
	    var = caar(vars);
	    val = cdar(vars);
	    vars = cdr(vars);
	    assocsym(var, val);
	}
    res = f_begin(arg3);
	ep = save;
	pp = save1;
	return (res);
    }
    //dummy
    return(TRUE);
}

int f_let_star(int arglist)
{
    int arg1, arg2, var, val, res, save;
    checkarg(LIST_TEST, "let*", car(arglist));
    arg1 = car(arglist);
    arg2 = cdr(arglist);

    save = ep;
    while (!nullp(arg1)) {
	var = caar(arg1);
	val = eval_cps(cadar(arg1));
	arg1 = cdr(arg1);
	assocsym(var, val);
    }
    res = f_begin(arg2);
    ep = save;
    return (res);
}

int f_letrec(int arglist)
{
    int arg1, arg2, fun, lam, lams, res, save;
    checkarg(LIST_TEST, "letrec", car(arglist));
    arg1 = car(arglist);
    arg2 = cdr(arglist);
    save = ep;
    lams = NIL;
    while (!nullp(arg1)) {
	fun = caar(arg1);
	lam = eval(cadar(arg1));
	lams = cons(lam, lams);
	arg1 = cdr(arg1);
	assocsym(fun, lam);
    }
    /* In the case of mutual recursion, each closure needs to share a common environment.
     * Therefore, set-clos and free-clos are disabled 
     * while the body of letrec is being executed.
     */
    letrec_flag = 1;
    res = f_begin(arg2);
    letrec_flag = 0;
    ep = save;
    return (res);
}

int f_do(int arglist)
{
    int arg1, arg2, arg3, var, init, update, temp, save, res;
    checkarg(LIST_TEST, "do", car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = cddr(arglist);

    save = ep;
    //initialize var
    temp = arg1;
    while (!nullp(temp)) {
	var = caar(temp);
	init = eval(cadr(car(temp)));
	assocsym(var, init);
	temp = cdr(temp);
    }
  loop:
    //check test
    if (eval(car(arg2)) != FAIL) {
	res = f_begin(cdr(arg2));
	ep = save;
	return (res);
    }
    //begin 
    f_begin(arg3);
    //update var
    temp = arg1;
    while (!nullp(temp)) {
	var = caar(temp);
	update = caddr(car(temp));
	bindsym(var, eval(update));
	temp = cdr(temp);
    }
    goto loop;
}

int f_delay(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    if(eqp(car(arg1),makesym("delay")))
        arg1 = f_delay(cdr(arg1));
    return (makepromise(arg1));
}

int f_force(int arglist)
{
    int arg1, body;
    arg1 = car(arglist);
    body = GET_BIND(arg1);
    //already forced return evaluated value
    if (GET_CAR(arg1) == TRUE)
	return (GET_CDR(arg1));
    else {
	SET_CAR(arg1, TRUE);
	SET_CDR(arg1, eval_cps(body));
	return (GET_CDR(arg1));
    }
}

int f_open_input_file(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "open-input-file", arglist);
    checkarg(STRING_TEST, "open-input-file", car(arglist));
    arg1 = car(arglist);

    return(makestm(fopen(GET_NAME(arg1), "r"),INPUT,GET_NAME(arg1)));
}


int f_open_output_file(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "open-output-file", arglist);
    checkarg(STRING_TEST, "open-output-file", car(arglist));
    arg1 = car(arglist);

    return(makestm(fopen(GET_NAME(arg1), "w"),OUTPUT,GET_NAME(arg1)));
}

int f_close_input_port(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "close-input-port", arglist);
    checkarg(STRING_TEST, "close-input-port", car(arglist));
    arg1 = car(arglist);
    fclose(GET_STM(arg1));
    return(TRUE);
}

int f_close_output_port(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "close-output-port", arglist);
    checkarg(STRING_TEST, "close-output-port", car(arglist));
    arg1 = car(arglist);
    fclose(GET_STM(arg1));
    return(TRUE);
}

int f_input_port_p(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "input-port?", arglist);
    arg1 = car(arglist);
    if(arg1 >= 0 && arg1 < HEAPSIZE && IS_STM(arg1) && GET_CDR(arg1) == INPUT)
        return(TRUE);
    else 
        return(FAIL);
}


int f_output_port_p(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "output-port?", arglist);
    arg1 = car(arglist);
    if(arg1 >= 0 && arg1 < HEAPSIZE && IS_STM(arg1) && GET_CDR(arg1) == OUTPUT)
        return(TRUE);
    else 
        return(FAIL);
}

int f_current_input_port(int arglist)
{
    checkarg(LEN0_TEST, "current-input-prt", arglist);
    return(input_stream);
}

int f_current_output_port(int arglist)
{
    checkarg(LEN0_TEST, "current-output-prt", arglist);
    return(output_stream);
}

int f_call_with_input_file(int arglist)
{
    int arg1,arg2,save,res;
    checkarg(STRING_TEST,"call-with-input-file", car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    save = input_stream;
    input_stream = makestm(fopen(GET_NAME(arg1), "r"),INPUT,GET_NAME(arg1));
    res = eval_cps(arg2);
    fclose(GET_STM(input_stream));
    input_stream = save;
    return(res);
}

int f_call_with_output_file(int arglist)
{
    int arg1,arg2,save,res;
    checkarg(STRING_TEST,"call-with-ouput-file",car(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    save = output_stream;
    output_stream = makestm(fopen(GET_NAME(arg1), "w"),OUTPUT,GET_NAME(arg1));
    res = eval_cps(arg2);
    fclose(GET_STM(output_stream));
    output_stream = save;
    return(res);
}

int f_load(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "load", arglist);
    checkarg(STRING_TEST, "load", car(arglist));
    arg1 = car(arglist);

    input_stream = makestm(fopen(GET_NAME(arg1), "r"),INPUT,GET_NAME(arg1));
    if (!GET_STM(input_stream))
	error(CANT_OPEN_ERR, "load", arg1);


    int exp;
    while (1) {
	exp = read();
	if (exp == EOF)
	    break;
	eval_cps(exp);
    }
    fclose(GET_STM(input_stream));
    input_stream = STDIN;
    return (TRUE);
}

int f_edwin(int arglist)
{
    int arg1, res;
    char str[SYMSIZE];

    arg1 = car(arglist);
    char *ed;
    if ((ed = getenv("EDITOR")) == NULL) {
	strcpy(str, "edwin");
    } else
	strcpy(str, ed);

    strcat(str, " ");
    strcat(str, GET_NAME(arg1));
    res = system(str);
    if (res == -1)
	error(CANT_OPEN_ERR, "ledit", arg1);
    f_load(arglist);
    return (TRUE);
}

int f_functionp(int arglist)
{
    checkarg(LEN1_TEST, "functionp", arglist);
    if (functionp(car(arglist)))
	return (TRUE);
    else
	return (FAIL);
}

int f_procedurep(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "procedure?", arglist);
    arg1 = car(arglist);
    if (arg1 >= 0 && arg1 < HEAPSIZE
	&& (IS_EXPR(arg1) || IS_SUBR(arg1) || IS_FSUBR(arg1)))
	return (TRUE);
    else
	return (FAIL);
}

int f_promisep(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "promise?", arglist);
    arg1 = car(arglist);
    if (arg1 >= 0 && arg1 < HEAPSIZE && IS_PROM(arg1))
	return (TRUE);
    else
	return (FAIL);
}


int f_call_cc(int arglist)
{
    int arg1, cont;
    checkarg(LEN1_TEST, "call/cc", arglist);
    arg1 = car(arglist); //lambda 
    cont = makecont();
    // (lambda (cont) continuation)
    return (apply_cps(arg1, list1(cont)));
}

int f_push(int arglist)
{
    push_cps(acc);
    return (T);
}

int f_pop(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    return (pops_cps(GET_INT(arg1)));
}

int f_bind(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    cps_bind(arg1);
    return (T);
}

int f_unbind(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    cps_unbind(GET_INT(arg1));
    return (acc);
}

int f_set_clos(int arglist)
{
    int arg1;
    if (letrec_flag)
	return (TRUE);

    arg1 = car(arglist);
    if (GET_REC(arg1) == 0) {
	SET_REC(arg1, 1);
	push_cps(ep);
	push_cps(arg1);
	if (GET_CDR(arg1) != NIL)
	    ep = GET_CDR(arg1);
    }
    return (TRUE);
}

int f_free_clos(int arglist)
{
    int expr;
    if (letrec_flag)
	return (acc);

    expr = pop_cps();
    SET_REC(expr, 0);
    ep = pop_cps();
    return (acc);
}



int f_transfer(int arglist)
{
    int arg1;
    arg1 = car(arglist);
    return (transfer(arg1));
}


int f_exec_cont(int arglist)
{
    int arg1, arg2;
    arg1 = car(arglist);	//continuation
    arg2 = cadr(arglist);	//argument to cont
    cp = GET_BIND(arg1);	//restore continuation;
    sp_cps = GET_CAR(arg1);	//restore stack
    ep = GET_CDR(arg1);		//restore environment
    acc = arg2;
    return (eval_cps(NIL));	//execute CPS
}

int f_environment(int arglist)
{
    print(ep);
    printf("\n");
    return (TRUE);
}

int f_vector(int arglist)
{
    return (makevec(arglist));
}

int f_vector_length(int arglist)
{
    int arg1;
    checkarg(VECTOR_TEST, "vector-length", car(arglist));
    arg1 = car(arglist);
    return (makeint(GET_CDR(arg1)));
}

int f_vector_ref(int arglist)
{
    int arg1, arg2;
    checkarg(VECTOR_TEST, "vector-ref", car(arglist));
    checkarg(INTEGER_TEST, "vector-ref", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    return (GET_VEC_ELT(arg1, GET_INT(arg2)));
}

int f_string_to_symbol(int arglist)
{
    int arg1;
    checkarg(STRING_TEST, "string->symbol", car(arglist));

    arg1 = car(arglist);
    return (makesym(GET_NAME(arg1)));
}


int f_symbol_to_string(int arglist)
{
    int arg1;
    checkarg(SYMBOL_TEST, "symbol->string", car(arglist));

    arg1 = car(arglist);
    return (makestr(GET_NAME(arg1)));
}


int f_charp(int arglist)
{
    checkarg(LEN1_TEST, "char?", arglist);
    return (characterp(car(arglist)));
}

int f_char_eqp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char=?", arglist);
    checkarg(CHAR_TEST, "char=?", car(arglist));
    checkarg(CHAR_TEST, "char=?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) == 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_ci_eqp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char-ci=?", arglist);
    checkarg(CHAR_TEST, "char-ci=?", car(arglist));
    checkarg(CHAR_TEST, "char-ci=?", cadr(arglist));
    arg1 = ci_char(car(arglist));
    arg2 = ci_char(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) == 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_lessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char<?", arglist);
    checkarg(CHAR_TEST, "char<?", car(arglist));
    checkarg(CHAR_TEST, "char<?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) < 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_ci_lessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char-ci<?", arglist);
    checkarg(CHAR_TEST, "char-ci<?", car(arglist));
    checkarg(CHAR_TEST, "char-ci<?", cadr(arglist));
    arg1 = ci_char(car(arglist));
    arg2 = ci_char(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) < 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_char_greaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char>?", arglist);
    checkarg(CHAR_TEST, "char>?", car(arglist));
    checkarg(CHAR_TEST, "char>?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) > 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_ci_greaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char-ci>?", arglist);
    checkarg(CHAR_TEST, "char-ci>?", car(arglist));
    checkarg(CHAR_TEST, "char-ci>?", cadr(arglist));
    arg1 = ci_char(car(arglist));
    arg2 = ci_char(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) > 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_char_eqlessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char<=?", arglist);
    checkarg(CHAR_TEST, "char<=?", car(arglist));
    checkarg(CHAR_TEST, "char<=?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) <= 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_char_ci_eqlessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char-ci<=?", arglist);
    checkarg(CHAR_TEST, "char-ci<=?", car(arglist));
    checkarg(CHAR_TEST, "char-ci<=?", cadr(arglist));
    arg1 = ci_char(car(arglist));
    arg2 = ci_char(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) <= 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_eqgreaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char>=?", arglist);
    checkarg(CHAR_TEST, "char>=?", car(arglist));
    checkarg(CHAR_TEST, "char>=?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) >= 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_ci_eqgreaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "char-ci>=?", arglist);
    checkarg(CHAR_TEST, "char-ci>=?", car(arglist));
    checkarg(CHAR_TEST, "char-ci>=?", cadr(arglist));
    arg1 = ci_char(car(arglist));
    arg2 = ci_char(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) >= 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_alphabetic_p(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "char-alphabetic?", arglist);
    arg1 = car(arglist);
    if (characterp(arg1) && isalpha(GET_NAME(arg1)) &&
	strcmp(GET_NAME(arg1), "space") != 0
	&& strcmp(GET_NAME(arg1), "newline") != 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_char_numeric_p(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "char-numeric?", arglist);
    arg1 = car(arglist);
    if (characterp(arg1) && isdigit(GET_NAME(arg1)))
	return (TRUE);
    else
	return (FAIL);
}


int f_char_whitespace_p(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "char-whitespace?", arglist);
    arg1 = car(arglist);
    if (characterp(arg1) && strcmp(GET_NAME(arg1), "space"))
	return (TRUE);
    else
	return (FAIL);
}



int f_char_upper_case_p(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "char-upper-case?", arglist);
    arg1 = car(arglist);
    if (characterp(arg1) && isupper(GET_NAME(arg1)) &&
	strcmp(GET_NAME(arg1), "space") != 0
	&& strcmp(GET_NAME(arg1), "newline") != 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_char_lower_case_p(int arglist)
{
    int arg1;
    checkarg(LEN1_TEST, "char-lower-case?", arglist);
    arg1 = car(arglist);
    if (characterp(arg1) && islower(GET_NAME(arg1)) &&
	strcmp(GET_NAME(arg1), "space") != 0
	&& strcmp(GET_NAME(arg1), "newline") != 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_stringp(int arglist)
{
    checkarg(LEN1_TEST, "string?", arglist);
    return (stringp(car(arglist)));
}

int f_make_string(int arglist)
{
    int arg1,arg2,n,i;
    char str[SYMSIZE],c;
    memset(str,0,SYMSIZE);
    if(length(arglist) == 1){
    arg1 = car(arglist);
    n = GET_INT(arg1);
    for(i=0;i<n;i++)
        str[i] = ' ';
    } else if(length(arglist) == 2){
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    n = GET_INT(arg1);
    c = GET_NAME(arg2)[2];
    for(i=0;i<n;i++)
    str[i] = c;
    } else 
        error(ARITY_ERR,"make-string",arglist);
    
    return(makestr(str));
}


int f_str_eqp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string=?", arglist);
    checkarg(STRING_TEST, "string=?", car(arglist));
    checkarg(STRING_TEST, "string=?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) == 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_str_ci_eqp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string-ci=?", arglist);
    checkarg(STRING_TEST, "string-ci=?", car(arglist));
    checkarg(STRING_TEST, "string-ci=?", cadr(arglist));
    arg1 = ci_str(car(arglist));
    arg2 = ci_str(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) == 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_str_lessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string<?", arglist);
    checkarg(STRING_TEST, "string<?", car(arglist));
    checkarg(STRING_TEST, "string<?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) < 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_str_ci_lessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string-ci<?", arglist);
    checkarg(STRING_TEST, "string-ci<?", car(arglist));
    checkarg(STRING_TEST, "string-ci<?", cadr(arglist));
    arg1 = ci_str(car(arglist));
    arg2 = ci_str(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) < 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_str_greaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string>?", arglist);
    checkarg(STRING_TEST, "string>?", car(arglist));
    checkarg(STRING_TEST, "string>?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) > 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_str_ci_greaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string-ci>?", arglist);
    checkarg(STRING_TEST, "string-ci>?", car(arglist));
    checkarg(STRING_TEST, "string-ci>?", cadr(arglist));
    arg1 = ci_str(car(arglist));
    arg2 = ci_str(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) > 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_str_eqlessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string<=?", arglist);
    checkarg(STRING_TEST, "string<=?", car(arglist));
    checkarg(STRING_TEST, "string<=?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) <= 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_str_ci_eqlessp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string-ci<=?", arglist);
    checkarg(STRING_TEST, "string-ci<=?", car(arglist));
    checkarg(STRING_TEST, "string-ci<=?", cadr(arglist));
    arg1 = ci_str(car(arglist));
    arg2 = ci_str(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) <= 0)
	return (TRUE);
    else
	return (FAIL);
}

int f_str_eqgreaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string>=?", arglist);
    checkarg(STRING_TEST, "string>=?", car(arglist));
    checkarg(STRING_TEST, "string>=?", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) >= 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_str_ci_eqgreaterp(int arglist)
{
    int arg1, arg2;
    checkarg(LEN2_TEST, "string-ci>=?", arglist);
    checkarg(STRING_TEST, "string-ci>=?", car(arglist));
    checkarg(STRING_TEST, "string-ci>=?", cadr(arglist));
    arg1 = ci_str(car(arglist));
    arg2 = ci_str(cadr(arglist));

    if (strcmp(GET_NAME(arg1), GET_NAME(arg2)) >= 0)
	return (TRUE);
    else
	return (FAIL);
}


int f_string_length(int arglist)
{
    int arg1, len;
    checkarg(STRING_TEST, "string-length", car(arglist));
    arg1 = car(arglist);
    len = strlen(GET_NAME(arg1));
    return (makeint(len));
}

int f_string_ref(int arglist)
{
    int arg1, arg2;
    char str[10];
    checkarg(STRING_TEST, "string-ref", car(arglist));
    checkarg(INTEGER_TEST, "string-ref", cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    str[0] = '#';
    str[1] = '\\';
    str[2] = GET_NAME_ELT(arg1, GET_INT(arg2));
    str[3] = 0;
    return (makechar(str));
}

int f_string_set(int arglist)
{
    int arg1, arg2, arg3;
    char str[SYMSIZE];
    checkarg(STRING_TEST, "string-set!", car(arglist));
    checkarg(INTEGER_TEST, "string-set!", cadr(arglist));
    checkarg(CHAR_TEST, "string-set!", caddr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = caddr(arglist);
    strcpy(str, GET_NAME(arg1));
    str[GET_INT(arg2)] = GET_NAME_ELT(arg3, 2);
    return (makestr(str));
}

int f_string_append(int arglist)
{
    char str[SYMSIZE];
    checkarg(STRING_TEST, "string-append", car(arglist));
    memset(str, 0, SYMSIZE);
    strcpy(str, GET_NAME(car(arglist)));
    arglist = cdr(arglist);
    while (!nullp(arglist)) {
	strcat(str, GET_NAME(car(arglist)));
	arglist = cdr(arglist);
    }
    return (makestr(str));
}

int f_string_to_list(int arglist)
{
    int arg1, i, res;
    char str[10];
    checkarg(STRING_TEST, "string->list", car(arglist));
    arg1 = car(arglist);
    i = strlen(GET_NAME(arg1)) - 1;
    res = NIL;
    str[0] = '#';
    str[1] = '\\';
    str[3] = 0;
    while (i >= 0) {
	str[2] = GET_NAME_ELT(arg1, i);
	res = cons(makechar(str), res);
	i--;
    }
    return (res);
}

int f_list_to_string(int arglist)
{
    int arg1, i;
    char str[SYMSIZE];
    checkarg(LIST_TEST, "list->string", car(arglist));
    arg1 = car(arglist);
    memset(str, 0, SYMSIZE);
    i = 0;
    while (!nullp(arg1)) {
	str[i] = GET_NAME_ELT(car(arg1), 2);
	i++;
	arg1 = cdr(arg1);
    }
    return (makestr(str));
}

int f_substring(int arglist)
{
    int arg1,arg2,arg3,i,j, start,end;
    char str1[SYMSIZE],str2[SYMSIZE];
    checkarg(STRING_TEST,"substring",car(arglist));
    checkarg(INTEGER_TEST,"substring",cadr(arglist));
    checkarg(INTEGER_TEST,"substring",caddr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    arg3 = caddr(arglist);
    memset(str1,0,SYMSIZE);
    memset(str2,0,SYMSIZE);
    strcpy(str1,GET_NAME(arg1));
    start = GET_INT(arg2);
    end = GET_INT(arg3);
    j = 0;
    for(i=start;i<end;i++){
        str2[j] = str1[i];
        j++;
    }
    return(makestr(str2));
}

int f_string_copy(int arglist)
{
    checkarg(STRING_TEST,"string-copy",car(arglist));
    return(makestr(GET_NAME(car(arglist))));
}

int f_string_fill(int arglist)
{
    int arg1,arg2,i,c;
    char str[SYMSIZE];
    checkarg(STRING_TEST,"string-fill!",car(arglist));
    checkarg(CHAR_TEST,"string-fill!",cadr(arglist));
    arg1 = car(arglist);
    arg2 = cadr(arglist);
    memset(str,0,SYMSIZE);
    strcpy(str,GET_NAME(arg1));
    c = GET_NAME(arg2)[3];
    i = 0;
    while(str[i] != 0){
        str[i] = c;
        i++;
    }
    return(makestr(str));
}

int f_exact_to_inexact(int arglist)
{
    checkarg(NUMBER_TEST,"exact->inexact",car(arglist));
    return(exact_to_inexact(car(arglist)));
}


int f_inexact_to_exact(int arglist)
{
    checkarg(NUMBER_TEST,"inexact->exact",car(arglist));
    return(inexact_to_exact(car(arglist)));
}

int f_number_to_string(int arglist)
{
    int arg1;
    char str[SYMSIZE];
    checkarg(NUMBER_TEST,"number->string",car(arglist));
    arg1 = car(arglist);
    memset(str,0,SYMSIZE);
    if(integerp(arg1)){
    sprintf(str,"%d",GET_INT(arg1));
    return(makestr(str));
    }else if(floatp(arg1));{
    sprintf(str,"%g", GET_FLT(arg1));
    return(makestr(str));
    }
    return(FAIL);
}


int f_string_to_number(int arglist)
{
    int arg1;
    char str[SYMSIZE];
    checkarg(STRING_TEST,"string->number",car(arglist));
    arg1 = car(arglist);
    memset(str,0,SYMSIZE);
    strcpy(str,GET_NAME(arg1));
    if(str[0] == 0)
        return(FAIL);
    else if(inttoken(str))
        return(makeint(atoi(str)));
    else if(flttoken(str))
        return(makeflt(atof(str)));
    else return(FAIL);
    
}
