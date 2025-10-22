/* Scheme R3RS
written by kenichi sasagawa 2025/10~
*/ 

#define HEAPSIZE 10000000
#define FREESIZE     1000
#define STACKSIZE  30000
#define SYMSIZE	256
#define BUFSIZE 256
#define NIL	0
#define T 8
#define TRUE 2
#define FAIL 3
#define HASHTBSIZE 107
#define NO HEAPSIZE+1

#define INT_FLAG    1073741824 //#b1000000000000000000000000000000
#define INT_MASK    1073741823 //#b0111111111111111111111111111111

#define DEBUG longjmp(buf, 2);

typedef enum tag {EMP,NUM,FLTN,STR,SYM,LIS,SUBR,FSUBR,EXPR,CONT,FEXPR,MACRO,BOOL,CHAR,VEC} tag;
typedef enum flag {FRE,USE} flag;


typedef struct cell {
	tag tag;
    flag flag;
	char *name;
	union{
    	int bind;
		int ( *subr) ();
		double fltnum;
    } val;
    int car;
    int cdr;
	int trace;
} cell;


typedef enum toktype {LPAREN,RPAREN,QUOTE,DOT,BACKQUOTE,COMMA,ATMARK,INTEGER,FLOAT,STRING,SYMBOL,BOOLEAN,CHARACTER,VECTOR,OTHER,FILEEND} toktype;
typedef enum backtrack {GO,BACK} backtrack;

typedef struct token {
	char ch;
    backtrack flag;
	toktype type;
    char buf[BUFSIZE];
} token;


#define GET_CAR(addr)		heap[addr].car
#define GET_CDR(addr)		heap[addr].cdr
#define GET_TR(addr)		heap[addr].trace
#define GET_INT(addr)		get_int(addr)
#define GET_FLT(addr)		((addr < HEAPSIZE && addr >0) ? heap[addr].val.fltnum: NIL)
#define GET_NAME(addr)		heap[addr].name
#define GET_TAG(addr)		heap[addr].tag
#define GET_BIND(addr)		heap[addr].val.bind
#define GET_SUBR(addr)		heap[addr].val.subr
#define GET_FLAG(addr)		heap[addr].flag
#define SET_TAG(addr,x)		heap[addr].tag = x
#define SET_CAR(addr,x)		heap[addr].car = x
#define SET_CDR(addr,x)		heap[addr].cdr = x
#define SET_TR(addr,x)		heap[addr].trace = x
#define SET_FLT(addr,x)		heap[addr].val.fltnum = x
#define	SET_BIND(addr,x)	heap[addr].val.bind = x
#define SET_NAME(addr,x)	heap[addr].name = (char *)malloc(SYMSIZE); strcpy(heap[addr].name,x);
#define SET_SUBR(addr,x)	heap[addr].val.subr = x
#define IS_SYMBOL(addr)		heap[addr].tag == SYM
#define IS_NUMBER(addr)		heap[addr].tag == NUM
#define IS_FLT(addr)		heap[addr].tag == FLTN
#define IS_STR(addr)		heap[addr].tag == STR
#define IS_BOOL(addr)		heap[addr].tag == BOOL
#define IS_CHAR(addr)	    heap[addr].tag == CHAR
#define IS_LIST(addr)		heap[addr].tag == LIS
#define IS_NIL(addr)        (addr == 0 || addr == 1)
#define IS_SUBR(addr)		heap[addr].tag == SUBR
#define IS_FSUBR(addr)		heap[addr].tag == FSUBR
#define IS_EXPR(addr)		heap[addr].tag == EXPR
#define IS_FEXPR(addr)		heap[addr].tag == FEXPR
#define IS_MACRO(addr)      heap[addr].tag == MACRO
#define IS_CONT(addr)		heap[addr].tag == CONT
#define IS_EMPTY(addr)		heap[addr].tag	== EMP
#define HAS_NAME(addr,x)	strcmp(heap[addr].name,x) == 0
#define SAME_NAME(addr1,addr2) strcmp(heap[addr1].name, heap[addr2].name) == 0
#define EQUAL_STR(x,y)		strcmp(x,y) == 0
#define MARK_CELL(addr)		heap[addr].flag = USE
#define NOMARK_CELL(addr)	heap[addr].flag = FRE
#define USED_CELL(addr)		heap[addr].flag == USE
#define FREE_CELL(addr)		heap[addr].flag == FRE


//------pointer----
int ep; //environment pointer
int hp; //heap pointer 
int sp; //stack pointer
int fc; //free counter
int ap; //arglist pointer
int cp; //continuation pointer
int cp1; //cp save;
int acc; //register 
int sp_csp; //stack pointer for CPS

//-------read--------
#define EOL		'\n'
#define TAB		'\t'
#define SPACE	' '
#define ESCAPE	033
#define NUL		'\0'

//-------error code---
#define CANT_FIND_ERR	1
#define ARG_SYM_ERR		2
#define ARG_INT_ERR		12
#define ARG_STR_ERR     13
#define ARG_NUM_ERR		3
#define ARG_LIS_ERR		4
#define ARG_BOOL_ERR    15
#define ARG_LEN0_ERR	5
#define ARG_LEN1_ERR	6
#define ARG_LEN2_ERR	7
#define ARG_LEN3_ERR	8
#define MALFORM_ERR		9
#define CANT_READ_ERR	10
#define CANT_OPEN_ERR   14
#define ILLEGAL_OBJ_ERR 11

//-------arg check code--
#define INTLIST_TEST    0
#define NUMLIST_TEST	1
#define SYMBOL_TEST		2
#define NUMBER_TEST		3
#define LIST_TEST		4
#define LEN0_TEST		5
#define LEN1_TEST		6
#define LEN2_TEST		7
#define LEN3_TEST		8
#define LENS1_TEST		9
#define LENS2_TEST		10
#define COND_TEST		11	
#define DEFLIST_TEST	12
#define SYMLIST_TEST	13
#define STRING_TEST	    14
#define BOOL_TEST       15


void initcell(void);
int freshcell(void);
void bindsym(int sym, int val);
void assocsym(int sym, int val);
int findsym(int sym);
int getsym(char *name, int index);
int addsym(char *name, int index);
int makesym1(char *name);
int hash(char *name);
void cellprint(int addr);
void heapdump(int start, int end);
void markoblist(void);
void markcell(int addr);
void gbcmark(void);
void gbcsweep(void);
void clrcell(int addr);
void gbc(void);
void checkgbc(void);
int car(int addr);
int cdr(int addr);
int cons(int car, int cdr);
int caar(int addr);
int cdar(int addr);
int cadr(int addr);
int caddr(int addr);
int cadar(int addr);
int assoc(int sym, int lis);
int length(int addr);
int list(int addr);
int append(int x, int y);
int makeint(int x);
int get_int(int x);
int integerp(int addr);
int makenum(int num);
int makesym(char *name);
int makebool(char *name);
int makefunc(int addr);
void gettoken(void);
int booltoken(char buf[]);
int chartoken(char buf[]);
int inttoken(char buf[]);
int flttoken(char buf[]);
int symboltoken(char buf[]);
int issymch(char c);
int read(void);
int readlist(void);
void print(int addr);
void printlist(int addr);
void princ(int addr);
int eval(int addr);
void bindarg(int lambda, int arglist);
void unbind(void);
int atomp(int addr);
int numberp(int addr);
int symbolp(int addr);
int listp(int addr);
int nullp(int addr);
int eqp(int addr1, int addr2);
int symnamep(int addr, char *name);
int evlis(int addr);
int apply(int func, int arg);
int apply_cps(int func, int args);
int subrp(int addr);
int fsubrp(int addr);
int functionp(int addr);
int lambdap(int addr);
int macrop(int addr);
void initsubr(void);
void defsubr(char *symname, int(*func)(int));
void deffsubr(char *symname, int(*func)(int));
void bindfunc(char *name, tag tag, int(*func)(int));
void bindmacro(int sym, int addr);
void push(int pt);
int pop(void);
void argpush(int addr);
void argpop(void);
void error(int errnum, char *fun, int arg);
void checkarg(int test, char *fun, int arg);
int isintlis(int arg);
int isnumlis(int arg);
int isdeflis(int arg);
int issymlis(int arg);

//---subr-------
int f_plus(int addr);
int f_difference(int addr);
int f_minus(int addr);
int	f_times(int addr);
int f_quotient(int addr);
int f_divide(int addr);
int f_max(int addr);
int f_min(int addr);
int f_abs(int addr);
int f_recip(int addr);
int f_remainder(int addr);
int f_expt(int addr);
int f_sqrt(int addr);
int f_sin(int addr);
int f_cos(int addr);
int f_tan(int addr);
int f_exit(int addr);
int f_heapdump(int addr);
int f_car(int addr);
int f_cdr(int addr);
int f_caar(int addr);
int f_cadr(int addr);
int f_cddr(int addr);
int f_caddr(int addr);
int f_cadddr(int addr);
int f_cons(int addr);
int f_length(int addr);
int f_list(int addr);
int f_reverse(int addr);
int f_assoc(int addr);
int f_member(int addr);
int f_append(int addr);
int f_nconc(int addr);
int f_rplaca(int addr);
int f_rplacd(int addr);
int f_mapcar(int addr);
int f_mapcon(int addr);
int f_map(int addr);
int f_nullp(int addr);
int f_atomp(int addr);
int f_numeqp(int addr);
int f_eq(int addr);
int f_equal(int addr);
int f_set(int addr);
int f_not(int addr);
int f_subst(int addr);
int f_functionp(int addr);
int f_procedurep(int addr);
int f_macrop(int addr);
int f_quote(int addr);
int f_setq(int addr);
int f_define(int addr);
int f_lambda(int addr);
int f_if(int addr);
int f_cond(int addr);
int f_and(int addr);
int f_or(int addr);
int f_let(int addr);
int f_let_star(int addr);
int f_load(int addr);
int f_edwin(int addr);
int f_numberp(int addr);
int f_rationalp(int addr);
int f_symbolp(int addr);
int f_listp(int addr);
int f_booleanp(int addr);
int f_greaterp(int addr);
int f_lessp(int addr);
int f_eqgreaterp(int addr);
int f_eqlessp(int addr);
int f_onep(int addr);
int f_zerop(int addr);
int f_minusp(int addr);
int f_gbc(int addr);
int f_eval(int addr);
int f_apply(int addr);
int f_read(int addr);
int f_readc(int addr);
int f_display(int addr);
int f_prin1(int addr);
int f_princ(int addr);
int f_newline(int addr);
int f_trace(int addr);
int f_untrace(int addr);
int f_gensym(int addr);
int f_step(int addr);
int f_putprop(int addr);
int f_get(int addr);
int f_begin(int addr);
int f_call_cc(int addr);
int f_push(int addr);
int f_pop(int addr);
int f_bind(int addr);
int f_unbind(int addr);
int f_transfer(int addr);
int f_exec_cont(int addr);
int f_apply_cps(int addr);
int f_eval_cps(int addr);

int quasi_transfer1(int x);
int quasi_transfer2(int x, int n);
int list1(int x);
int list2(int x, int y);
int list3(int x, int y, int z);
int transfer(int addr);
int execute(int addr);
int eval_cps(int addr);