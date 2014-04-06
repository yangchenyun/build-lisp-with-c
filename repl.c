#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"
#define DEBUG 0

void apply_opt(char* opt, long* result, long operand) {
  if (strcmp(opt, "+") == 0 || strcmp(opt, "add") == 0) {
    *result += operand;
  }
  if (strcmp(opt, "-") == 0 || strcmp(opt, "sub") == 0) {
    *result -= operand;
  }
  if (strcmp(opt, "*") == 0 || strcmp(opt, "mul") == 0) {
    *result *= operand;
  }
  if (strcmp(opt, "/") == 0 || strcmp(opt, "div") == 0) {
    *result /= operand;
  }
  if (strcmp(opt, "%") == 0) {
    *result %= operand;
  }
};

long eval(mpc_ast_t* t) {
  // the termination condition
  if (strstr(t->tag, "number")) { return atoi(t->contents); }

  // assumption, t has the structure <operator> <expression> <expression>
  char* opt = t->children[1]->contents;

  // store 3rd children as first operand
  long result = eval(t->children[2]);

  // apply operation to all tailing expressions
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    apply_opt(opt, &result, eval(t->children[i]));
    i++;
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
      operator   : '+' | '-' | '*' | '/' | '%' | \"add\" | \"sub\" | \"mul\" | \"div\"; \
      expr : <number> | '(' <operator> <expr>+ ')';\
      program    : /^/ <operator> <expr>+ /$/;\
      number     : /-?[0-9]+(\\.[0-9]+)?/; \
      ",
      Number, Operator, Expression, Prog);

  mpc_result_t r;

  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.1");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char *input = readline("lispy> ");
    add_history(input);
    if (mpc_parse("<stdin>", input, Prog, &r)) {
      if (DEBUG) { mpc_ast_print(r.output); }
      printf("%li\n", eval(r.output));
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, Number, Operator, Expression, Prog);
  return 0;
}
