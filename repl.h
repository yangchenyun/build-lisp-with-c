#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

enum LTYPE { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};

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

// Evaluation / Data Transformation
Lval* buildin_op(Lval* l, char* op);
Lval* lval_eval_sexpr(Lval* v);
Lval* lval_eval(Lval* v);

// API with AST
Lval* lval_read_num(mpc_ast_t* t);
Lval* lval_read(mpc_ast_t* t);
