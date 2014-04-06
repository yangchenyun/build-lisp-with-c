#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"

int main(int argc, const char *argv[])
{
  mpc_parser_t *Prog  = mpc_new("program");
  mpc_parser_t *Expression = mpc_new("expression");
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Operator  = mpc_new("operator");

  mpca_lang(MPC_LANG_DEFAULT,
      " \
      operator   : '+' | '-' | '*' | '/' | '%' | \"add\" | \"sub\" | \"mul\" | \"div\"; \
      expression : <number> | '(' <operator> <expression>+ ')';\
      program    : /^/ <operator> <expression>+ /$/;\
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
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(4, Number, Operator, Expression, Prog);
  return 0;
}
