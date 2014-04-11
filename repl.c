#include <readline/readline.h>
#include <readline/history.h>
#include "mpc.h"
#include "repl.h"
#define DEBUG 0

char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}
// helper functions to create num / errors
Lval* lval_num(int num) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_NUM;
  v->num = num;
  return v;
};

Lval* lval_err(char* fmt, ...) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_ERR;

  va_list va;
  va_start(va, fmt);
  v->err = malloc(512);
  vsnprintf(v->err, 511, fmt, va);
  v->err = realloc(v->err, strlen(v->err) + 1);
  va_end(va);

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
  v->buildin = func;
  return v;
};

Lval* lval_lambda(Lval* formals, Lval* body) {
  Lval* v = malloc(sizeof(Lval));
  v->type = LVAL_FUN;

  v->buildin = NULL;
  v->env = lenv_new();
  v->formals = formals;
  v->body = body;

  return v;
}

Lval* lval_copy(Lval* l) {
  Lval* v = malloc(sizeof(Lval));
  v->type = l->type;

  switch (v->type) {
    case LVAL_NUM:
      v->num = l->num;
      break;
    case LVAL_FUN:
      if (v->buildin) {
        v->buildin = l->buildin;
      } else {
        v->buildin = NULL;
        v->env = lenv_copy(l->env);
        v->formals = lval_copy(l->formals);
        v->body = lval_copy(l->body);
      }
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
      if (!v->buildin) {
        lenv_del(v->env);
        lval_del(v->formals);
        lval_del(v->body);
      }
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
    case LVAL_FUN:
      if (v->buildin) {
        fprintf(out, "<build>");
      } else {
        fprintf(out, "(lambda ");
        lval_print(v->formals);
        fputc(' ', out);
        lval_print(v->body);
        fputc(')', out);
      }
      break;
  }
}

void lval_println(Lval* v) {
  FILE* out = DEBUG ? stderr : stdout;
  lval_print(v);
  fputc('\n', out);
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

Lval* lval_eval_sexpr(Lenv* e, Lval* v) {
  // replace children with evaluated result
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
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
  if (f->type != LVAL_FUN) {
    lval_del(f); lval_del(v);
    return lval_err("Expect the first element to be a Function, Got %s", ltype_name(f->type));
  }

  // apply the operation for the rest of list
  Lval* result = f->buildin(e, v);
  lval_del(f);
  return result;
};

Lval* lval_eval(Lenv* e, Lval* v) {
  if (v->type == LVAL_SYM) {
    Lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->type == LVAL_SEXPR) {
    return lval_eval_sexpr(e, v);
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

Lenv* lenv_new(void) {
  Lenv* e = malloc(sizeof(Lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  e->status = NULL;
  return e;
};

void lenv_del(Lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }

  free(e->syms);
  free(e->status);
  free(e->vals);
  free(e);
};

Lval* lenv_get(Lenv* e, Lval* k) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(k->sym, e->syms[i]) == 0) {
      return lval_copy(e->vals[i]); // internal immutable data structure
    }
  }

  return lval_err("unbound symbol %s", k->sym);
};

void lenv_val_print(Lenv* e, Lval* k) {
  FILE* out = DEBUG ? stderr : stdout;
  Lval* v = lenv_get(e, k);
  lval_print(k);
  fputc(':', out);
  fputc(' ', out);
  lval_print(v);
};

void lenv_val_println(Lenv* e, Lval* k) {
  FILE* out = DEBUG ? stderr : stdout;
  lenv_val_print(e, k);
  fputc('\n', out);
};
void lenv_print(Lenv* e) {
  FILE* out = DEBUG ? stderr : stdout;
  for (int i = 0; i < e->count; i++) {
    fprintf(out, "%s => [%s]", e->syms[i], ltype_name(e->vals[i]->type));
    lval_print(e->vals[i]);
    fputc('\n', out);
  };
};

Lenv* lenv_copy(Lenv* e) {
  return lenv_new(); 
}

bool lenv_put(Lenv* e, Lval* k, Lval* v, bool status) {
  assert(k->type == LVAL_SYM);

  // for symbol exists in the env
  for (int i = 0; i < e->count; i++) {
    if (strcmp(k->sym, e->syms[i]) == 0) {
      if (e->status[i]) { return ERR_BUILDIN; }
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return 0;
    }
  }

  // new symbols
  e->count++;
  e->syms = realloc(e->syms, sizeof(char*) * e->count);
  e->vals = realloc(e->vals, sizeof(Lval*) * e->count);
  e->status = realloc(e->status, sizeof(bool*) * e->count);

  e->status[e->count - 1] = status;
  e->vals[e->count - 1] = lval_copy(v);
  e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
  strcpy(e->syms[e->count - 1], k->sym);

  return 0;
};

void lenv_add_buildin(Lenv* e, char* name, Lbuildin func) {
  Lval* k = lval_sym(name);
  Lval* v = lval_fun(func);
  lenv_put(e, k, v, true);
  lval_del(k); lval_del(v);
}

Lval* buildin_add(Lenv* e, Lval* l) { return buildin_op(e, l, "+"); }
Lval* buildin_sub(Lenv* e, Lval* l) { return buildin_op(e, l, "-"); }
Lval* buildin_mul(Lenv* e, Lval* l) { return buildin_op(e, l, "*"); }
Lval* buildin_div(Lenv* e, Lval* l) { return buildin_op(e, l, "/"); }
Lval* buildin_mod(Lenv* e, Lval* l) { return buildin_op(e, l, "%"); }

void lenv_init_buildins(Lenv* e) {
  /* List Functions */
  lenv_add_buildin(e, "list", buildin_list);
  lenv_add_buildin(e, "head", buildin_head);
  lenv_add_buildin(e, "tail",  buildin_tail);
  lenv_add_buildin(e, "eval", buildin_eval);
  lenv_add_buildin(e, "join",  buildin_join);
  lenv_add_buildin(e, "cons", buildin_cons);
  lenv_add_buildin(e, "len",  buildin_len);
  lenv_add_buildin(e, "init",  buildin_init);

  /* Mathematical Functions */
  lenv_add_buildin(e, "+", buildin_add);
  lenv_add_buildin(e, "-", buildin_sub);
  lenv_add_buildin(e, "*", buildin_mul);
  lenv_add_buildin(e, "/", buildin_div);
  lenv_add_buildin(e, "%", buildin_mod);

  /* Variable Functions */
  lenv_add_buildin(e, "def", buildin_def);
  lenv_add_buildin(e, "exit", buildin_exit);
  lenv_add_buildin(e, "lambda", buildin_lambda);
}

Lval* buildin_def(Lenv* e, Lval* l) {
  LASSERT_TYPE("def", l, 0, LVAL_QEXPR);

  Lval* syms = l->cell[0];

  for (int i = 0; i < syms->count; i++) {
    LASSERT_TYPE("def", syms, i, LVAL_SYM);
  }

  LASSERT(l, syms->count == l->count - 1,
      "Function 'def' symbols and values don't match, symbols are %d, values are %d",
      syms->count, l->count - 1);

  for (int i = 0; i < syms->count; i++) {
    if (lenv_put(e, syms->cell[i], l->cell[i + 1], false) == ERR_BUILDIN) {
      return lval_err("symbol declaration failed, %s names are taken", syms->cell[i]->sym);
    }
  }

  return lval_sexp();
};

Lval* buildin_exit(Lenv* e, Lval* l) {
  LASSERT_TYPE("exit", l, 0, LVAL_NUM);
  exit(l->cell[0]->num);
};

Lval* buildin_lambda(Lenv* e, Lval* l) {
  LASSERT_NUM("lambda", l, 2);
  LASSERT_TYPE("lambda", l, 0, LVAL_QEXPR);
  LASSERT_TYPE("lambda", l, 1, LVAL_QEXPR);

  for (int i = 0; i < l->cell[0]->count; i++) {
    LASSERT(l, l->cell[0]->cell[i]->type == LVAL_SYM,
        "cannot define non-symbol as formal arguments. Expect %s, Got %s",
        ltype_name(LVAL_SYM), ltype_name(l->cell[0]->cell[i]->type));
  }

  Lval* formals = lval_pop(l, 0);
  Lval* body = lval_pop(l, 0);
  lval_del(l);

  return lval_lambda(formals, body);
};

Lval* buildin_list(Lenv* e, Lval* l) {
  l->type = LVAL_QEXPR;
  return l;
};

Lval* buildin_head(Lenv* e, Lval* l) {
  LNONEMPTY(l);
  LASSERT_NUM("head", l, 1);
  LASSERT_TYPE("head", l, 0, LVAL_QEXPR);

  Lval* ql = lval_take(l, 0); // extract the qexpr
  while (ql->count > 1) { lval_del(lval_pop(ql, 1)); }
  return ql;
};

Lval* buildin_tail(Lenv* e, Lval* l) {
  LNONEMPTY(l);
  LASSERT_NUM("tail", l, 1);
  LASSERT_TYPE("tail", l, 0, LVAL_QEXPR);

  Lval* ql = lval_take(l, 0); // extract the qexpr
  lval_del(lval_pop(ql, 0));
  return ql;
};

Lval* buildin_join(Lenv* e, Lval* l) {
  for (int i = 0; i < l->count; i++) {
    LASSERT_TYPE("join", l, i, LVAL_QEXPR);
  }

  Lval* ql = lval_pop(l, 0);

  while(l->count != 0) {
    lval_join(ql, lval_pop(l, 0));
  }

  lval_del(l);
  return ql;
}

Lval* buildin_cons(Lenv* e, Lval* l) {
  LASSERT_NUM("cons", l, 2);
  LASSERT_TYPE("cons", l, 1, LVAL_QEXPR);

  Lval* a = lval_pop(l, 0);
  Lval* ql = lval_pop(l, 0);
  lval_insert(ql, a, 0);
  lval_del(l);
  return ql;
};

Lval* buildin_eval(Lenv* e, Lval* l) {
  LNONEMPTY(l);
  LASSERT_NUM("eval", l, 1);

  Lval* ql = lval_take(l, 0);
  ql->type = LVAL_SEXPR;
  return lval_eval(e, ql);
};

Lval* buildin_len(Lenv* e, Lval* l) {
  LASSERT_NUM("len", l, 1);
  LASSERT_TYPE("len", l, 0, LVAL_QEXPR);

  Lval* ql = lval_take(l, 0);
  int len = ql->count;
  lval_del(ql);
  return lval_num(len);
};

Lval* buildin_init(Lenv* e, Lval* l) {
  LNONEMPTY(l);
  LASSERT_NUM("init", l, 1);
  LASSERT_TYPE("init", l, 0, LVAL_QEXPR);

  Lval* ql = lval_take(l, 0); // extract the qexpr
  lval_del(lval_pop(ql, ql->count - 1));
  return ql;
};

Lval* buildin_op(Lenv* e, Lval* l, char* op) {
  for (int i = 0; i < l->count; i++) {
    if (l->cell[i]->type != LVAL_NUM) {
      Lval* err = lval_err("Function '%s' passed in incorrect type for args %d. Got %s, Expect %s",
          op, i, ltype_name(l->cell[i]->type), ltype_name(LVAL_NUM));
      lval_del(l);
      return err;
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
      symbol  : /[a-zA-Z0-9_+\\-*\\/%\\\\=<>!&]+/; \
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

  Lenv* e = lenv_new();
  lenv_init_buildins(e);

  while (1) {
    char *input = readline("lispy> ");
    add_history(input);
    if (mpc_parse("<stdin>", input, Prog, &r)) {
      if (DEBUG) { mpc_ast_print(r.output); }
      Lval* x = lval_eval(e, lval_read(r.output));
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
