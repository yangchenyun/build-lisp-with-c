#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"
#define DEBUG 0

// Lisp Values for evaluation
typedef struct {
  int type;
  long num;
  int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// helper functions to create num / errors
lval lval_num(int num) {
  lval v;
  v.type = LVAL_NUM;
  v.num = num;
  return v;
};

lval lval_err(int err) {
  lval v;
  v.type = LVAL_ERR;
  v.err = err;
  return v;
};

void lval_print(lval v) {
  switch (v.type) {
    /* In the case the type is a number print it, then 'break' out of the switch. */
    case LVAL_NUM: printf("%li\n", v.num); break;

    /* In the case the type is an error */
    case LVAL_ERR:
      /* Check What exact type of error it is and print it */
      if (v.err == LERR_DIV_ZERO) { printf("Error: Division By Zero\n"); }
      if (v.err == LERR_BAD_OP)   { printf("Error: Invalid Operator\n"); }
      if (v.err == LERR_BAD_NUM)  { printf("Error: Invalid Number\n"); }
    break;
  }
}

lval eval_binary_op(char* opt, lval v) {
  // propgate the result out
  if (v.type == LVAL_ERR) { return v; }
  if (strcmp(opt, "-") == 0) { return lval_num(-v.num); }
  return lval_err(LERR_BAD_OP);
}

lval eval_op(char* opt, lval x, lval y) {
  // error propagation
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  if (strcmp(opt, "+") == 0 || strcmp(opt, "add") == 0) {
    return lval_num(x.num + y.num);
  }
  if (strcmp(opt, "-") == 0 || strcmp(opt, "sub") == 0) {
    return lval_num(x.num - y.num);
  }
  if (strcmp(opt, "*") == 0 || strcmp(opt, "mul") == 0) {
    return lval_num(x.num * y.num);
  }
  if (strcmp(opt, "/") == 0 || strcmp(opt, "div") == 0) {
    return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
  }
  if (strcmp(opt, "%") == 0) {
    return lval_num(x.num % y.num);
  }
  if (strcmp(opt, "^") == 0) {
    return lval_num(pow(x.num, y.num));
  }
  if (strcmp(opt, "min") == 0) {
    return x.num > y.num ? lval_num(y.num) : lval_num(x.num);
  }
  if (strcmp(opt, "max") == 0) {
    return x.num < y.num ? lval_num(y.num) : lval_num(x.num);
  }

  return lval_err(LERR_BAD_OP);
};

lval eval(mpc_ast_t* t) {
  // the termination condition
  if (strstr(t->tag, "number")) {
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  // assumption, t has the structure <operator> <expression> <expression>
  char* opt = t->children[1]->contents;

  // store 3rd children as first operand
  lval result = eval(t->children[2]);

  // with one operand
  if (t->children_num == 4) { result = eval_binary_op(opt, result); }

  if (t->children_num >= 4) { // with two operands
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
      // apply operation to all tailing expressions
      result = eval_op(opt, result, eval(t->children[i]));
      i++;
    }
  }

  return result;
};

int main(int argc, const char *argv[])
{
  mpc_parser_t *Prog  = mpc_new("program");
  mpc_parser_t *Expression = mpc_new("expr");
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator  = mpc_new("operator");

  mpca_lang(MPC_LANG_DEFAULT,
      " \
      operator   : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"min\" | \"max\"; \
      expr : <number> | '(' <operator> <expr>+ ')';\
      program    : /^/ <operator> <expr>+ /$/;\
      number     : /-?[0-9]+(\\.[0-9]+)?/; \
      ",
      Number, Operator, Expression, Prog);

  mpc_result_t r;

  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.2");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char *input = readline("lispy> ");
    add_history(input);
    if (mpc_parse("<stdin>", input, Prog, &r)) {
      if (DEBUG) { mpc_ast_print(r.output); }
      lval_print(eval(r.output));
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, Number, Operator, Expression, Prog);
  return 0;
}
