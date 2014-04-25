#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <assert.h>

enum LTYPE { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN};
enum ENVERR { ERR_BUILDIN = 1 };
#define LASSERT(l, cond, fmt, ...) if (!(cond)) { \
    Lval* err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(l); \
    return err; };
#define LNONEMPTY(l) LASSERT(l, l->cell[0]->count != 0, "{} is not allowed!")
#define LASSERT_NUM(fname, l, i) LASSERT(l, l->count == i, "Function %s passed with wrong arguments. Expect %d, Got %d", fname, i, l->count);
#define LASSERT_TYPE(fname, l, i, expect) \
  LASSERT(l, l->cell[i]->type == expect, "Function %s is passed in wrong type of arguments at %d. Expect %s, Got %s", fname, i, ltype_name(expect), ltype_name(l->cell[i]->type))

char* ltype_name(int t);

typedef struct Lval Lval;
typedef struct Lenv Lenv;
typedef Lval* (*Lbuildin)(Lenv*, Lval*);

// Lisp Values for evaluation
struct Lval {
  enum LTYPE type;

  long num; // for number

  char* err; // for error messages
  char* sym; // for symbol

  /*  Function */
  Lbuildin buildin;
  Lenv* env;
  Lval* formals;
  Lval* body;

  // for sexp
  int count;
  struct Lval** cell;
};

// Construction methods
Lval* lval_num(int num);
Lval* lval_err(char* fmt, ...);
Lval* lval_sym(char* s);
Lval* lval_sexp(void);
Lval* lval_qexp(void);
Lval* lval_fun(Lbuildin func);
Lval* lval_lambda(Lval* formals, Lval* body);
Lval* lval_copy(Lval* l);

// Destruction methods
void lval_del(Lval* v);

// Print Methods
void lval_print(Lval* v);
void lval_expr_print(Lval* v, char open, char close);
void lval_println(Lval* v);

// Data Manipulation
Lval* lval_add(Lval* v, Lval* x);
Lval* lval_pop(Lval* v, int i); // pop the child at index i
Lval* lval_take(Lval* v, int i); // take elements and leave out the rest
Lval* lval_join(Lval* v, Lval* u);
Lval* lval_insert(Lval* v, Lval* a, int i);

// Comparison
int lval_eq(Lval* v, Lval* w);

struct Lenv {
  int count;
  Lenv* par;
  char** syms;
  Lval** vals;
  bool* status; // freeze status
};

Lenv* lenv_new(void);
void lenv_del(Lenv* e);
Lval* lenv_get(Lenv* e, Lval* k);
bool lenv_put(Lenv* e, Lval* k, Lval* v, bool status); // 1 = freeze, 0 = mutable
bool lenv_def(Lenv* e, Lval* k, Lval* v, bool status); // 1 = freeze, 0 = mutable
void lenv_add_buildin(Lenv* e, char* name, Lbuildin func);
void lenv_init_buildins(Lenv* e);
void lenv_val_print(Lenv* e, Lval* k);
void lenv_val_println(Lenv* e, Lval* k);
void lenv_print(Lenv* e);
Lenv* lenv_copy(Lenv* e);
char* lenv_status_name(int t);

// buildin functions
Lval* buildin_op(Lenv* e, Lval* l, char* op);
Lval* buildin_list(Lenv* e, Lval* l); // (list 1 2 3 4)   => {1 2 3 4}
Lval* buildin_head(Lenv* e, Lval* l); // (head {1 2 3})   => {1}
Lval* buildin_tail(Lenv* e, Lval* l); // (tail {1 2 3})   => {2 3}
Lval* buildin_join(Lenv* e, Lval* a); // (join {1} {2 3}) => {1 2 3}
Lval* buildin_eval(Lenv* e, Lval* l); // (eval {+ 1 2})   => 3
Lval* buildin_cons(Lenv* e, Lval* l); // (cons 1 {2 3})   => {1, 2, 3}
Lval* buildin_len(Lenv* e, Lval* l);  // (len {1 2 3})    => 3
Lval* buildin_init(Lenv* e, Lval* l); // (init {1 2 3})   => {1 2}

Lval* buildin_add(Lenv* e, Lval* l);
Lval* buildin_sub(Lenv* e, Lval* l);
Lval* buildin_mul(Lenv* e, Lval* l);
Lval* buildin_div(Lenv* e, Lval* l);
Lval* buildin_mod(Lenv* e, Lval* l);

Lval* buildin_var(Lenv* e, Lval* l, char* func);
Lval* buildin_def(Lenv* e, Lval* l);
Lval* buildin_put(Lenv* e, Lval* l);
Lval* buildin_exit(Lenv* e, Lval* l);
Lval* buildin_lambda(Lenv* e, Lval* l);

Lval* buildin_ord(Lenv* e, Lval* l, char* op);
Lval* buildin_lt(Lenv* e, Lval* l);
Lval* buildin_lteq(Lenv* e, Lval* l);
Lval* buildin_gt(Lenv* e, Lval* l);
Lval* buildin_gteq(Lenv* e, Lval* l);

Lval* buildin_cmp(Lenv* e, Lval* l, char* op);
Lval* buildin_eq(Lenv* e, Lval* l);
Lval* buildin_neq(Lenv* e, Lval* l);

Lval* buildin_if(Lenv* e, Lval* l);

// Evaluation / Data Transformation
Lval* lval_eval_sexpr(Lenv* e, Lval* v);
Lval* lval_eval(Lenv* e, Lval* v);
Lval* lval_call(Lenv* e, Lval* f, Lval* l);

// API with AST
Lval* lval_read_num(mpc_ast_t* t);
Lval* lval_read(mpc_ast_t* t);
