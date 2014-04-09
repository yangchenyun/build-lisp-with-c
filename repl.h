#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

enum LTYPE { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};
#define LASSERT(l, cond, err) if (cond) { lval_del(l); return lval_err(err); };
#define LNONEMPTY(l) LASSERT(l, l->cell[0]->count == 0, "{} is not allowed!")
#define LARGNUM(l, i) LASSERT(l, l->count != i, "Function passed with wrong arguments!");

// Lisp Values for evaluation
typedef struct Lval {
  enum LTYPE type;

  long num; // for number

  char* err; // for error messages
  char* sym; // for symbol

  // for sexp
  int count;
  struct Lval** cell;
} Lval;

// Construction methods
Lval* lval_num(int num);
Lval* lval_err(char* m);
Lval* lval_sym(char* s);
Lval* lval_sexp(void);
Lval* lval_qexp(void);

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
