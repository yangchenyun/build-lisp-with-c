#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

enum LTYPE { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR};
#define LASSERT(l, cond, err) if (cond) { lval_del(l); return lval_err(err); };
#define LASSERT_NONEMPTY_L(l) LASSERT(l, l->cell[0]->count == 0, "{} is not allowed!")

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

// Evaluation / Data Transformation
Lval* buildin(Lval* l, char* func);
Lval* buildin_op(Lval* l, char* op);
// Takes one or more arguments and returns a new Q-Expression
Lval* buildin_list(Lval* l);
// Takes a Q-Expression and returns a Q-Expression with only of the first element
Lval* buildin_head(Lval* l);
// Takes a Q-Expression and returns a Q-Expression with the first element removed
Lval* buildin_tail(Lval* l);
// Takes one or more Q-Expressions and returns a Q-Expression of them conjoined together
Lval* buildin_join(Lval* a);
// Takes a Q-Expression and evaluates it as if it were a S-Expression
Lval* buildin_eval(Lval* l);
Lval* lval_eval_sexpr(Lval* v);
Lval* lval_eval(Lval* v);

// API with AST
Lval* lval_read_num(mpc_ast_t* t);
Lval* lval_read(mpc_ast_t* t);
