#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"

enum LTYPE { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR};
enum LERROR { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

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

void lval_print(Lval* v);
void lval_sexpr_print(Lval* v, char open, char close);

// pop the child at index i
Lval* lval_pop(Lval* v, int i);
// take elements and leave out the rest
Lval* lval_take(Lval* v, int i);

Lval* lval_eval_sexpr(Lval* v);
Lval* lval_eval(Lval* v);

Lval* buildin_op(Lval* l, char* op);
