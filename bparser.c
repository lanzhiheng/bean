#include <limits.h>
#include <assert.h>
#include "bstring.h"
#include "bhash.h"
#include "bparser.h"

#define MAX_ARGS 16 // MAX args of function
#define MIN_EXPR_SIZE 16
#define get_tk_precedence(ls) (symbol_table[ls->t.type].lbp)

static dynamic_expr * init_dynamic_expr(bean_State * B UNUSED) {
  dynamic_expr * target = malloc(sizeof(dynamic_expr));
  target -> es = malloc(sizeof(struct expr *) * MIN_EXPR_SIZE);
  target -> size = MIN_EXPR_SIZE;
  target -> count = 0;
  return target;
}

static void add_element(bean_State * B, dynamic_expr * target, struct expr * expression) {
  if (target -> count + 1 > target -> size) {
    size_t oldSize = (target -> size) * sizeof(struct expr *);
    size_t newSize = oldSize * 2;
    target -> es = beanM_realloc_(B, target -> es, oldSize, newSize);

    if (target -> es) {
      target -> size = newSize;
    }
  }
  target -> es[target -> count++] = expression;
}

/* maximum number of local variables per function (must be smaller
   than 250, due to the bytecode format) */
#define MAXVARS		200

#define hasmultret(kind)		((kind) == VCALL || (kind) == VVARARG)

/*
** maximum number of upvalues in a closure (both C and Lua). (Value
** must fit in a VM register.)
*/
#define MAXUPVAL	255

/* because all strings are unified by the scanner, the parser
   can use pointer equality for string equality */
#define eqstr(a,b)	((a) == (b))

/* semantic error */
void beanK_semerror (LexState *ls, const char *msg) {
  ls->t.type = 0;  /* remove "near <token>" from final message */ // TODO: Why
  beanX_syntaxerror(ls, msg);
}

/* static void error_expected (LexState *ls, int token) { */
/*   beanX_syntaxerror(ls, */
/*       beanO_pushfstring(ls->B, "%s expected", beanX_token2str(ls, token))); */
/* } */

/*
** Check that next token is 'c'.
*/
/* static void check (LexState *ls, int c) { */
/*   if (ls->t.type != c) */
/*     error_expected(ls, c); */
/* } */

/*
** Test whether next token is 'c'; if so, skip it.
*/
static bool testnext (LexState *ls, int c) {
  if (ls->t.type == c) {
    beanX_next(ls);
    return true;
  }
  else return false;
}

/*
** Check that next token is 'c' and skip it.
*/
/* static void checknext (LexState *ls, int c) { */
/*   check(ls, c); */
/*   beanX_next(ls); */
/* } */


#define check_condition(ls, c, msg) { if(!c) beanX_syntaxerror(ls, msg); }

/*
** grep "ORDER OPR" if you change these enums  (ORDER OP)
*/
static expr * parse_statement(LexState *ls, bindpower rbp);
static expr * infix (LexState *ls, expr * left);
static expr * function_call (LexState *ls, expr * left);
static expr * string (LexState *ls, expr * exp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_STRING;
  ep -> sval = ls->t.seminfo.ts;
  beanX_next(ls);
  return ep;
}

static expr* num(LexState *ls, expr * exp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  switch(ls->t.type) {
    case(TK_INT): {
      ep -> type = EXPR_NUM;
      ep -> ival = ls->t.seminfo.i;
      break;
    }
    case(TK_FLT): {
      ep -> type = EXPR_FLOAT;
      ep -> nval = ls->t.seminfo.r;
      break;
    }
    default:
      beanK_semerror(ls, "Not the valid number.");
  }
  beanX_next(ls);
  return ep;
}

static expr* boolean(LexState *ls, expr * exp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_BOOLEAN;
  switch(ls->t.type) {
    case(TK_TRUE): {
      ep -> bval = true;
      break;
    }
    case(TK_FALSE): {
      ep -> bval = false;
      break;
    }
    default:
      beanK_semerror(ls, "Not the valid boolean value.");
  }
  beanX_next(ls);
  return ep;
}

static expr* left_paren(LexState *ls, expr *exp UNUSED) {
  testnext(ls, TK_LEFT_PAREN);
  expr * ep = malloc(sizeof(expr));
  ep = parse_statement(ls, BP_LOWEST);
  testnext(ls, TK_RIGHT_PAREN);
  return ep;
}

static expr* variable(LexState *ls, expr *exp UNUSED) {
  expr * ep = malloc(sizeof(expr));

  Token preToken = ls->pre;
  Token token = ls->t;
  beanX_next(ls);

  if (preToken.type == TK_VAR) { // variable definition
    ep -> type = EXPR_DVAR;
    ep -> var.name = token.seminfo.ts;

    if (ls->t.type == TK_ASSIGN) {
      beanX_next(ls);
      ep -> var.value = parse_statement(ls, BP_LOWEST);
    } else {
      ep -> var.value = NULL;
    }
  } else {
    ep -> type = EXPR_GVAR;
    ep -> gvar.name = token.seminfo.ts;
  }
  return ep;
}

static expr* return_exp(LexState *ls, expr * exp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  beanX_next(ls);
  ep -> type = EXPR_RETURN;

  // TODO: support to return the null value
  if (ls -> t.type == TK_RIGHT_BRACE) beanK_semerror(ls, "You have to set the return value after the return statement\n");

  ep -> ret.ret_val = parse_statement(ls, BP_LOWEST);
  return ep;
}

static Proto * parse_prototype(LexState *ls) {
  Proto * p = malloc(sizeof(Proto));

  if (!testnext(ls, TK_NAME)) {
    beanK_semerror(ls, "Expect function name in prototype!");
  }

  TString *name = ls->t.seminfo.ts;
  p -> name = name;
  p -> args = malloc(sizeof(TString) * MAX_ARGS);
  p -> arity = 0;

  if (!testnext(ls, TK_LEFT_PAREN)) {
    beanK_semerror(ls, "Expect '(' in prototype!");
  }

  if (testnext(ls, TK_RIGHT_PAREN)) {
    return p;
  }

  do {
    if (testnext(ls, TK_NAME)) {
      p->args[p->arity++] = ls->t.seminfo.ts;
    } else {
      beanK_semerror(ls, "Expect token name in args list!");
    }
  } while(testnext(ls, TK_COMMA));

  if (!testnext(ls, TK_RIGHT_PAREN)) {
    beanK_semerror(ls, "Expect '(' in prototype!");
  }

  return p;
}

static expr * parse_definition(LexState *ls) {
  beanX_next(ls);
  Function * f = malloc(sizeof(Function));
  f -> p = parse_prototype(ls);
  f -> body = init_dynamic_expr(ls->B);
  testnext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    add_element(ls->B, f->body, parse_statement(ls, BP_LOWEST));
  }

  testnext(ls, TK_RIGHT_BRACE);
  expr * ex = malloc(sizeof(expr));
  ex->type = EXPR_FUN;
  ex->fun = f;
  return ex;
}

symbol symbol_table[] = {
  /* arithmetic operators */
  { "and", BP_LOGIC_AND, NULL, NULL },
  { "break", BP_NONE, NULL, NULL },
  { "else", BP_NONE, NULL, NULL },
  { "elseif", BP_NONE, NULL, NULL },
  { "+", BP_TERM, NULL, infix },
  { "-", BP_TERM, NULL, infix },
  { "*", BP_FACTOR, NULL, infix },
  { "/", BP_FACTOR, NULL, infix },
  { ":", BP_NONE, NULL, NULL },
  { ",", BP_NONE, NULL, NULL },
  { ";", BP_NONE, NULL, NULL },
  { "{", BP_NONE, NULL, NULL },
  { "}", BP_NONE, NULL, NULL },
  { "(", BP_CALL, left_paren, function_call },
  { ")", BP_NONE, NULL, NULL },
  { "[", BP_DOT, NULL, infix },
  { "]", BP_NONE, NULL, NULL },
  { ".", BP_DOT, NULL, infix },
  { "false", BP_NONE, boolean, NULL },
  { "for", BP_NONE, NULL, NULL },
  { "func", BP_NONE, NULL, NULL },
  { "if", BP_NONE, NULL, NULL },
  { "in", BP_CONDITION, NULL, NULL },
  { "var", BP_NONE, NULL, NULL },
  { "nil", BP_NONE, NULL, NULL },
  { "not", BP_CONDITION, NULL, NULL },
  { "or", BP_LOGIC_OR, NULL, NULL },
  { "return", BP_NONE, return_exp, NULL },
  { "true", BP_NONE, boolean, NULL },
  { "while", BP_NONE, NULL, NULL },
  { "==", BP_EQUAL, NULL, infix },
  { "=", BP_ASSIGN, NULL, infix },
  { ">=", BP_CMP, NULL, infix },
  { ">", BP_CMP, NULL, infix },
  { "<=", BP_CMP, NULL, infix },
  { "<", BP_CMP, NULL, infix },
  { "~=", BP_CMP, NULL, infix },
  { "<<", BP_BIT_SHIFT, NULL, infix },
  { ">>", BP_BIT_SHIFT, NULL, infix },
  { "<eof>", BP_NONE, NULL, NULL },
  { "<number>", BP_NONE, num, NULL },
  { "<integer>", BP_NONE, num, NULL },
  { "<name>", BP_NONE, variable, NULL },
  { "<string>", BP_NONE, string, NULL },
};

static expr * function_call (LexState *ls, expr * left) {
  expr * func_call = malloc(sizeof(expr));
  func_call -> type = EXPR_CALL;
  func_call -> call.callee = left;
  func_call->call.args = init_dynamic_expr(ls->B);

  if (testnext(ls, TK_RIGHT_PAREN)) return func_call;

  do {
    add_element(ls->B, func_call->call.args, parse_statement(ls, BP_LOWEST));
  } while(testnext(ls, TK_COMMA));

  testnext(ls, TK_RIGHT_PAREN);
  return func_call;
}

static expr * infix (LexState *ls, expr * left) {
  expr * temp = malloc(sizeof(expr));
  temp -> type = EXPR_BINARY;
  temp -> infix.op = ls->pre.type;
  temp -> infix.left = left;
  temp -> infix.right = parse_statement(ls, symbol_table[ls->pre.type].lbp);
  if (temp -> infix.op == TK_LEFT_BRACKET) testnext(ls, TK_RIGHT_BRACKET);
  return temp;
}

static expr * parse_branch(struct LexState *ls, bindpower rbp) {
  testnext(ls, TK_IF);
  expr * tree = malloc(sizeof(expr));
  tree -> type = EXPR_BRANCH;
  tree -> branch.condition = parse_statement(ls, rbp);
  tree -> branch.if_body = init_dynamic_expr(ls->B);
  testnext(ls, TK_LEFT_BRACE);
  while (ls->t.type != TK_RIGHT_BRACE) {
    add_element(ls->B, tree -> branch.if_body, parse_statement(ls, rbp));
  }
  testnext(ls, TK_RIGHT_BRACE);

  if (ls->t.type == TK_ELSE) {
    testnext(ls, TK_ELSE);

    if (ls -> t.type == TK_IF) { // else if ... => else { if ... }
      tree -> branch.else_body = init_dynamic_expr(ls->B);
      add_element(ls->B, tree -> branch.else_body, parse_branch(ls, rbp));
    } else {
      tree -> branch.else_body = init_dynamic_expr(ls->B);
      testnext(ls, TK_LEFT_BRACE);
      while (ls->t.type != TK_RIGHT_BRACE) {
        add_element(ls->B, tree -> branch.else_body, parse_statement(ls, rbp));
      }
      testnext(ls, TK_RIGHT_BRACE);
    }
  } else {
    tree -> branch.else_body = NULL;
  }
  return tree;
}

static expr * parse_while(struct LexState *ls, bindpower rbp) {
  beanX_next(ls);

  expr * tree = malloc(sizeof(expr));
  tree -> type = EXPR_LOOP;
  tree -> loop.condition = parse_statement(ls, rbp);
  tree -> loop.body = init_dynamic_expr(ls->B);
  testnext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    add_element(ls->B, tree->loop.body, parse_statement(ls, rbp));
  }

  testnext(ls, TK_RIGHT_BRACE);
  return tree;
}

static expr * parse_break(struct LexState *ls UNUSED, bindpower rbp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_BREAK;
  beanX_next(ls);
  return ep;
}

static expr * parse_array(struct LexState *ls UNUSED, bindpower rbp UNUSED) {
  beanX_next(ls);
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_ARRAY;
  ep -> array = init_dynamic_expr(ls->B);

  if (testnext(ls, TK_RIGHT_BRACKET)) return ep;
  do {
    add_element(ls->B, ep->array, parse_statement(ls, BP_LOWEST));
  } while(testnext(ls, TK_COMMA));
  testnext(ls, TK_RIGHT_BRACKET);
  return ep;
}

static expr * parse_hash(struct LexState *ls UNUSED, bindpower rbp UNUSED) {
  beanX_next(ls);
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_HASH;
  ep -> hash = init_dynamic_expr(ls->B);
  if (testnext(ls, TK_RIGHT_BRACE)) return ep;

  do {
    add_element(ls->B, ep->hash, parse_statement(ls, BP_LOWEST));
    testnext(ls, TK_COLON);
    add_element(ls->B, ep->hash, parse_statement(ls, BP_LOWEST));
  } while(testnext(ls, TK_COMMA));

  testnext(ls, TK_RIGHT_BRACE);
  return ep;
}

static expr * parse_statement(struct LexState *ls, bindpower rbp) {
  if (ls->t.type == TK_BREAK) {
    return parse_break(ls, rbp);
  }

  if (ls->t.type == TK_WHILE) {
    return parse_while(ls, rbp);
  }

  if (ls->t.type == TK_IF) {
    return parse_branch(ls, rbp);
  }

  if (ls->t.type == TK_FUNCTION) { // Define an function
    return parse_definition(ls);
  }

  if (ls->t.type == TK_LEFT_BRACKET) {
    return parse_array(ls, rbp);
  }

  if (ls->t.type == TK_LEFT_BRACE) {
    return parse_hash(ls, rbp);
  }

  if (ls->t.type == TK_VAR) { // Define an variable
    beanX_next(ls);

    if (ls->t.type != TK_NAME) {
      beanK_semerror(ls, "Must have a variable name.");
    }
  }

  expr * tree = symbol_table[ls->t.type].nud(ls, NULL);

  while (rbp < get_tk_precedence(ls)) {
    Token token = ls->t;
    beanX_next(ls);
    tree = symbol_table[token.type].led(ls, tree);
  }

  return tree;
};

static void skip_semicolon(LexState * ls) { // Skip all the semicolon
  while (ls -> t.type == TK_SEMI) beanX_next(ls);
}

static void parse_program(LexState * ls) {
  skip_semicolon(ls);
  expr * ex = parse_statement(ls, BP_LOWEST);
  skip_semicolon(ls);
  eval(ls->B, ex);
}

void bparser(LexState * ls) {
  beanX_next(ls);

  while (ls -> current != EOZ) {
    parse_program(ls);
  }
}
