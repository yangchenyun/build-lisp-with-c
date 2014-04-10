#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"
#include "repl.h"
#define DEBUG 0

// helper functions to create num / errors
Lval* lval_num(int num) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_NUM;
  v->num = num;
  return v;
};

Lval* lval_err(char* m) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
};

Lval* lval_sym(char* s) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

Lval* lval_sexp(void) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

Lval* lval_qexp(void) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

Lval* lval_fun(Lbuildin func) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_FUN;
  v->fun = func;
  return v;
};

Lval* lval_copy(Lval* l) {
  Lval* v = malloc(sizeof(Lval));
  v->type = l->type;

  switch (v->type) {
    case LVAL_NUM:
      v->num = l->num;
      break;
    case LVAL_FUN:
      v->fun = l->fun;
      break;
    case LVAL_ERR:
      v->err = malloc(strlen(l->err) + 1); strcpy(v->err, l->err);
      break;
    case LVAL_SYM:
      v->err = malloc(strlen(l->sym) + 1); strcpy(v->sym, l->sym);
      break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      v->count = l->count;
      v->cell = malloc(sizeof(Lval*) * l->count);
      for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_copy(l->cell[i]);
      }
      break;
  };

  return v;
};

void lval_del(Lval* v) {
  switch (v->type) {
    case LVAL_NUM:
    case LVAL_FUN:
      break;
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      free(v->cell);
      break;
  }

  free(v);
};

void lval_expr_print(Lval* v, char open, char close) {
  FILE* out = DEBUG ? stderr : stdout;

  fputc(open, out);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);

    // print spaces except trailing element
    if (i != (v->count - 1)) {
      fputc(' ', out);
    }
  }

  fputc(close, out);
}

void lval_print(Lval* v) {
  FILE* out = DEBUG ? stderr : stdout;
  switch (v->type) {
    case LVAL_NUM: fprintf(out, "%li", v->num); break;
    case LVAL_ERR: fprintf(out, "ERROR: %s", v->err); break;
    case LVAL_SYM: fprintf(out, "%s", v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    case LVAL_FUN: fprintf(out, "<function>"); break;
  }
}

void lval_println(Lval* v) {
  lval_print(v);
  putchar('\n');
}

// add x to the sexp or qexp
Lval* lval_add(Lval* v, Lval* x) {
  assert(v->type == LVAL_SEXPR || v->type == LVAL_QEXPR);
  v->count++;
  v->cell = realloc(v->cell, sizeof(Lval*) * v->count);
  v->cell[v->count - 1] = x;
  return v;
};

Lval* lval_read_num(mpc_ast_t* t) {
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

Lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  Lval* x = NULL;
  // root node
  if (strcmp(t->tag, ">") == 0) { x = lval_sexp(); }
  // sexpr node
  if (strstr(t->tag, "sexpr")) { x = lval_sexp(); }
  // qexpr node
  if (strstr(t->tag, "qexpr")) { x = lval_qexp(); }

  // add any valid child
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }


  return x;
};


Lval* lval_eval_sexpr(Lval* v) {
  // replace children with evaluated result
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  // propagate the errors
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) {
      return lval_take(v, i);
    }
  }

  if (v->count == 0) { return v; }
  if (v->count == 1) { return lval_take(v, 0); }

  Lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression Does not start with symbol!");
  }

  // apply the operation for the rest of list
  Lval* result = buildin(v, f->sym);
  lval_del(f);
  return result;
};

Lval* lval_eval(Lval* v) {
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(v);
  }

  return v;
};

// take at the child out of v at index i
Lval* lval_take(Lval* v, int i) {
  Lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
};

// pop the child at index i
Lval* lval_pop(Lval* v, int i) {
  // hold that variable
  Lval* x = v->cell[i];

  // shift the after array over
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(Lval*) * (v->count - i - 1));

  // reduce the count
  v->count--;
  v->cell = realloc(v->cell, sizeof(Lval*) * (v->count));

  return x;
};

// insert the child at index i
Lval* lval_insert(Lval* v, Lval* a, int i) {
  assert(v->type == LVAL_SEXPR || v->type == LVAL_QEXPR);
  v->count++;
  v->cell = realloc(v->cell, sizeof(Lval*) * v->count);
  // move for the position of new element
  memmove(&v->cell[i + 1], &v->cell[i], sizeof(Lval*) * (v->count - i - 1));

  v->cell[i] = a;
  return v;

  // hold that variable
  Lval* x = v->cell[i];

  // shift the after array over
  memmove(&v->cell[i], &v->cell[i + 1], sizeof(Lval*) * (v->count - i - 1));

  // reduce the count
  v->count--;
  v->cell = realloc(v->cell, sizeof(Lval*) * (v->count));

  return x;
};

Lval* lval_join(Lval* v, Lval* u) {
  while(u->count != 0) {
    lval_add(v, lval_pop(u, 0));
  }

  lval_del(u);
  return v;
};

Lval* buildin_list(Lval* l) {
  l->type = LVAL_QEXPR;
  return l;
};

Lval* buildin_head(Lval* l) {
  LNONEMPTY(l);
  LARGNUM(l, 1);
  LASSERT(l, l->cell[0]->type != LVAL_QEXPR, "Function 'head' only accept Qexpr!");

  Lval* ql = lval_take(l, 0); // extract the qexpr
  return lval_take(ql, 0);
};

Lval* buildin_tail(Lval* l) {
  LNONEMPTY(l);
  LARGNUM(l, 1);
  LASSERT(l, l->cell[0]->type != LVAL_QEXPR, "Function 'tail' only accept Qexpr!");

  Lval* ql = lval_take(l, 0); // extract the qexpr
  lval_del(lval_pop(ql, 0));
  return ql;
};

Lval* buildin_join(Lval* l) {
  for (int i = 0; i < l->count; i++) {
    LASSERT(l, l->cell[i]->type != LVAL_QEXPR, "Function 'join' only accept Qexprs!");
  }

  Lval* ql = lval_pop(l, 0);

  while(l->count != 0) {
    lval_join(ql, lval_pop(l, 0));
  }

  lval_del(l);
  return ql;
}

Lval* buildin_cons(Lval* l) {
  LARGNUM(l, 2);
  LASSERT(l, l->cell[1]->type != LVAL_QEXPR, "Function 'cons' could only apply to Qexpr!");

  Lval* a = lval_pop(l, 0);
  Lval* ql = lval_pop(l, 0);
  lval_insert(ql, a, 0);
  lval_del(l);
  return ql;
};

Lval* buildin_eval(Lval* l) {
  LNONEMPTY(l);
  LARGNUM(l, 1);

  Lval* ql = lval_take(l, 0);
  ql->type = LVAL_SEXPR;
  return lval_eval(ql);
};

Lval* buildin_len(Lval* l) {
  LARGNUM(l, 1);
  LASSERT(l, l->cell[0]->type != LVAL_QEXPR, "Function 'cons' could only apply to Qexpr!");

  Lval* ql = lval_take(l, 0);
  int len = ql->count;
  lval_del(ql);
  return lval_num(len);
};

Lval* buildin_init(Lval* l) {
  LNONEMPTY(l);
  LARGNUM(l, 1);
  LASSERT(l, l->cell[0]->type != LVAL_QEXPR, "Function 'init' only accept Qexpr!");

  Lval* ql = lval_take(l, 0); // extract the qexpr
  lval_del(lval_pop(ql, ql->count - 1));
  return ql;
};

Lval* buildin(Lval* l, char* fn) {
  if (strcmp(fn, "list") == 0) { return buildin_list(l); }
  if (strcmp(fn, "head") == 0) { return buildin_head(l); }
  if (strcmp(fn, "tail") == 0) { return buildin_tail(l); }
  if (strcmp(fn, "join") == 0) { return buildin_join(l); }
  if (strcmp(fn, "cons") == 0) { return buildin_cons(l); }
  if (strcmp(fn, "eval") == 0) { return buildin_eval(l); }
  if (strcmp(fn, "len") == 0) { return buildin_len(l); }
  if (strcmp(fn, "init") == 0) { return buildin_init(l); }
  if (strstr("-+*^%/ min max add sub mul div", fn)) {
    return buildin_op(l, fn);
  }
  lval_del(l);
  return lval_err("Unknown functions!");
};

Lval* buildin_op(Lval* l, char* op) {
  for (int i = 0; i < l->count; i++) {
    if (l->cell[i]->type != LVAL_NUM) {
      lval_del(l);
      return lval_err("Cannot operator on non number!");
    }
  }

  Lval* x = lval_pop(l, 0); // Lval to hold the result

  if (l->count == 0) {
    if (strcmp(op, "-") == 0)  {
      x->num = -x->num;
    } else {
      lval_del(x);
      char *emsg = malloc(sizeof(char) * 256);
      strcat(emsg, "Invalid Operands for ");
      strcat(emsg, op);
      x = lval_err(emsg);
    }
  }

  while (l->count != 0) {
    Lval* y = lval_pop(l, 0);
    if (DEBUG) {
      fprintf(stderr, "%s %ld %ld\n", op, x->num, y->num);
      lval_println(l);
    }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "^") == 0) { x->num = pow(x->num, y->num); }
    if (strcmp(op, "%") == 0) { x->num %= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
        lval_del(x); lval_del(y); lval_del(l);
        x = lval_err("Division By Zero!");
        break;
      } else {
        x->num /= y->num;
      }
    }

    lval_del(y);
  }

  lval_del(l);
  return x;
};

int main(int argc, const char *argv[])
{
  mpc_parser_t *Prog  = mpc_new("program");
  mpc_parser_t *Expr = mpc_new("expr");
  mpc_parser_t *Sexpr = mpc_new("sexpr");
  mpc_parser_t *Qexpr = mpc_new("qexpr");
  mpc_parser_t *Number = mpc_new("number");
  mpc_parser_t *Symbol  = mpc_new("symbol");

  mpca_lang(MPC_LANG_DEFAULT,
      " \
      symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/; \
      number  : /-?[0-9]+(\\.[0-9]+)?/; \
      expr    : <number> | <symbol> | <sexpr> | <qexpr> ;\
      sexpr   : '(' <expr>* ')';\
      qexpr   : '{' <expr>* '}';\
      program : /^/ <expr>* /$/;\
      ",
      Number, Symbol, Expr, Sexpr, Qexpr, Prog);

  mpc_result_t r;

  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.5");
  puts("Press Ctrl+c to Exit\n");

  while (1) {
    char *input = readline("lispy> ");
    add_history(input);
    if (mpc_parse("<stdin>", input, Prog, &r)) {
      if (DEBUG) { mpc_ast_print(r.output); }
      Lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  }

  mpc_cleanup(6, Number, Symbol, Expr, Sexpr, Qexpr, Prog);
  return 0;
}
