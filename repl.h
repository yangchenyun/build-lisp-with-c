#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

enum LTYPE { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN};
#define LASSERT(l, cond, err) if (cond) { lval_del(l); return lval_err(err); };
#define LNONEMPTY(l) LASSERT(l, l->cell[0]->count == 0, "{} is not allowed!")
#define LARGNUM(l, i) LASSERT(l, l->count != i, "Function passed with wrong arguments!");

typedef struct Lval Lval;
typedef struct Lenv Lenv;
typedef Lval* (*Lbuildin)(Lenv*, Lval*);

// Lisp Values for evaluation
struct Lval {
  enum LTYPE type;

  long num; // for number

  char* err; // for error messages
  char* sym; // for symbol
  Lbuildin fun;

  // for sexp
  int count;
  struct Lval** cell;
};

// Construction methods
Lval* lval_num(int num);
Lval* lval_err(char* m);
Lval* lval_sym(char* s);
Lval* lval_sexp(void);
Lval* lval_qexp(void);
Lval* lval_fun(Lbuildin func);
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

struct Lenv {
  int count;
  char** syms;
  Lval** vals;
};
Lenv* lenv_new(void);
void lenv_del(Lenv* e);
Lval* lenv_get(Lenv* e, Lval* k);
void lenv_put(Lenv* e, Lval* k, Lval* v);
void lenv_add_buildin(Lenv* e, char* name, Lbuildin func);
void lenv_init_buildins(Lenv* e);
// buildin functions
Lval* buildin(Lval* l, char* func);
Lval* buildin_op(Lval* l, char* op);
Lval* buildin_list(Lval* l); // (list 1 2 3 4)   => {1 2 3 4}
Lval* buildin_head(Lval* l); // (head {1 2 3})   => {1}
Lval* buildin_tail(Lval* l); // (tail {1 2 3})   => {2 3}
Lval* buildin_join(Lval* a); // (join {1} {2 3}) => {1 2 3}
Lval* buildin_eval(Lval* l); // (eval {+ 1 2})   => 3
Lval* buildin_cons(Lval* l); // (cons 1 {2 3})   => {1, 2, 3}
Lval* buildin_len(Lval* l);  // (len {1 2 3})    => 3
Lval* buildin_init(Lval* l); // (init {1 2 3})   => {1 2}

// Evaluation / Data Transformation
Lval* lval_eval_sexpr(Lval* v);
Lval* lval_eval(Lval* v);

// API with AST
Lval* lval_read_num(mpc_ast_t* t);
Lval* lval_read(mpc_ast_t* t);
