#include <limits.h>
#include <assert.h>
#include "bstring.h"
#include "berror.h"
#include "bhash.h"
#include "bparser.h"

#define MAX_ARGS 16 // MAX args of function
#define MIN_EXPR_SIZE 16
#define get_tk_precedence(ls) (symbol_table[ls->t.type].lbp)

static dynamic_expr * init_dynamic_expr(bean_State * B UNUSED) {
  dynamic_expr * target = malloc(sizeof(dynamic_expr));
  target -> es = calloc(sizeof(struct expr *), MIN_EXPR_SIZE);
  target -> size = MIN_EXPR_SIZE;
  target -> count = 0;
  return target;
}

static void add_element(bean_State * B, dynamic_expr * target, struct expr * expression) {
  if (target -> count + 1 > target -> size) {
    size_t oldSize = (target -> size) * sizeof(struct expr *);
    size_t newSize = oldSize * 2;
    target->es = beanM_realloc_(B, target -> es, oldSize, newSize);

    if (target->es) {
      for (uint32_t i = target->size; i < newSize; i++) target->es[i] = NULL;

      target->size = newSize;
    }
  }
  target -> es[target -> count++] = expression;
}

/* maximum number of local variables per function (must be smaller
   than 250, due to the bytecode format) */
#define MAXVARS		200

/*
** maximum number of upvalues in a closure (both C and Lua). (Value
** must fit in a VM register.)
*/
#define MAXUPVAL	255

/* because all strings are unified by the scanner, the parser
   can use pointer equality for string equality */
#define eqstr(a,b)	((a) == (b))

/*
** Test whether next token is 'c'; if so, skip it.
*/
static bool checknext (LexState *ls, int c) {
  if (ls->t.type == c) {
    beanX_next(ls);
    return true;
  }
  else return false;
}

//  Check that next token is 'c' and skip it.
static void testnext (LexState *ls, int c) {
  if (ls->t.type == c) {
    beanX_next(ls);
  } else {
    syntax_error(ls, "SyntaxError: An syntax error occur");
  }
}

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
    case(TK_NUM): {
      ep -> type = EXPR_NUM;
      ep -> nval = ls->t.seminfo.n;
      break;
    }
    default:
      syntax_error(ls, "Not the valid number.");
  }
  beanX_next(ls);
  return ep;
}

static expr* nil(LexState *ls, expr * exp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_NIL;
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
      syntax_error(ls, "Not the valid boolean value.");
  }
  beanX_next(ls);
  return ep;
}

static expr* self(LexState *ls, expr *exp UNUSED) {
  checknext(ls, TK_SELF);
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_SELF;
  return ep;
}

static expr* left_paren(LexState *ls, expr *exp UNUSED) {
  checknext(ls, TK_LEFT_PAREN);
  expr * ep = malloc(sizeof(expr));
  ep = parse_statement(ls, BP_LOWEST);
  checknext(ls, TK_RIGHT_PAREN);
  return ep;
}

static expr* variable(LexState *ls, expr *exp UNUSED) {
  expr * ep = malloc(sizeof(expr));

  Token preToken = ls->pre;
  Token token = ls->t;
  beanX_next(ls);
  bool isGlobal = preToken.type == TK_GLOBAL;

  if (ls->t.type == TK_ASSIGN && preToken.type != TK_DOT) {
    ep->type = EXPR_DVAR;
    ep->var.name = token.seminfo.ts;

    beanX_next(ls);
    ep->var.value = parse_statement(ls, BP_LOWEST);
  } else {
    ep->type = EXPR_GVAR;
    ep->var.name = token.seminfo.ts;
    ep->var.value = NULL;
  }
  ep->var.global = isGlobal;

  return ep;
}

static expr* return_exp(LexState *ls, expr * exp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  beanX_next(ls);
  ep -> type = EXPR_RETURN;

  // TODO: support to return the null value
  if (ls -> t.type == TK_RIGHT_BRACE) syntax_error(ls, "You have to set the return value after the return statement");

  ep -> ret.ret_val = parse_statement(ls, BP_LOWEST);
  return ep;
}

static Proto * parse_prototype(LexState *ls) {
  Token preToken = ls->pre;
  Proto * p = malloc(sizeof(Proto));
  beanX_next(ls);

  TString * name = NULL;

  bool assign = preToken.type == TK_COLON || preToken.type == TK_ASSIGN;
  bool immediate = preToken.type == TK_LEFT_PAREN;

  if (!checknext(ls, TK_NAME)) {
    if (!assign && !immediate) {
      syntax_error(ls, "Expect function name in prototype!");
    }
  } else {
    name = ls->t.seminfo.ts;
  }

  p -> assign = assign;
  p -> name = name;
  p -> args = malloc(sizeof(TString) * MAX_ARGS);
  p -> arity = 0;

  testnext(ls, TK_LEFT_PAREN);

  if (checknext(ls, TK_RIGHT_PAREN)) {
    return p;
  }

  do {
    testnext(ls, TK_NAME);
    p->args[p->arity++] = ls->t.seminfo.ts;
  } while(checknext(ls, TK_COMMA));

  testnext(ls, TK_RIGHT_PAREN);

  return p;
}

static expr * parse_definition(LexState *ls) {
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

static expr * unary(LexState *ls, expr * exp UNUSED) {
  expr * temp = malloc(sizeof(expr));
  temp -> type = EXPR_UNARY;
  temp -> unary.op = ls->t.type;
  beanX_next(ls);
  temp -> unary.val = parse_statement(ls, BP_LOWEST);
  return temp;
}

static expr * parse_array(struct LexState *ls UNUSED, expr * exp UNUSED) {
  beanX_next(ls);
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_ARRAY;
  ep -> array = init_dynamic_expr(ls->B);

  if (checknext(ls, TK_RIGHT_BRACKET)) return ep;
  do {
    add_element(ls->B, ep->array, parse_statement(ls, BP_LOWEST));
  } while(checknext(ls, TK_COMMA));
  checknext(ls, TK_RIGHT_BRACKET);
  return ep;
}

static expr * parse_hash(struct LexState *ls UNUSED, expr * exp UNUSED) {
  beanX_next(ls);
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_HASH;
  ep -> hash = init_dynamic_expr(ls->B);
  if (checknext(ls, TK_RIGHT_BRACE)) return ep;

  do {
    add_element(ls->B, ep->hash, parse_statement(ls, BP_LOWEST));
    checknext(ls, TK_COLON);
    add_element(ls->B, ep->hash, parse_statement(ls, BP_LOWEST));
  } while(checknext(ls, TK_COMMA));

  checknext(ls, TK_RIGHT_BRACE);
  return ep;
}

symbol symbol_table[] = {
  /* arithmetic operators */
  { "and", BP_LOGIC_AND, NULL, infix },
  { "break", BP_NONE, NULL, NULL },
  { "else", BP_NONE, NULL, NULL },
  { "elseif", BP_NONE, NULL, NULL },
  { "+", BP_TERM, unary, infix },
  { "-", BP_TERM, unary, infix },
  { "*", BP_FACTOR, NULL, infix },
  { "/", BP_FACTOR, NULL, infix },
  { "&", BP_BIT_AND, NULL, infix },
  { "|", BP_BIT_OR, NULL, infix },
  { "^", BP_BIT_XOR, NULL, infix },
  { "%", BP_FACTOR, NULL, infix },
  { ":", BP_NONE, NULL, NULL },
  { ",", BP_NONE, NULL, NULL },
  { ";", BP_NONE, NULL, NULL },
  { "!", BP_NONE, unary, NULL },
  { "{", BP_NONE, parse_hash, NULL },
  { "}", BP_NONE, NULL, NULL },
  { "(", BP_CALL, left_paren, function_call },
  { ")", BP_NONE, NULL, NULL },
  { "[", BP_DOT, parse_array, infix },
  { "]", BP_NONE, NULL, NULL },
  { ".", BP_DOT, NULL, infix },
  { "false", BP_NONE, boolean, NULL },
  { "for", BP_NONE, NULL, NULL },
  { "fn", BP_NONE, NULL, NULL },
  { "if", BP_NONE, NULL, NULL },
  { "self", BP_NONE, self, NULL },
  { "in", BP_CONDITION, NULL, NULL },
  { "typeof", BP_NONE, unary, NULL },
  { "global", BP_NONE, NULL, NULL },
  { "nil", BP_NONE, nil, NULL },
  { "not", BP_NONE, unary, NULL },
  { "or", BP_LOGIC_OR, NULL, infix },
  { "return", BP_NONE, return_exp, NULL },
  { "true", BP_NONE, boolean, NULL },
  { "while", BP_NONE, NULL, NULL },
  { "==", BP_EQUAL, NULL, infix },
  { "=", BP_ASSIGN, NULL, infix },
  { ">=", BP_CMP, NULL, infix },
  { ">", BP_CMP, NULL, infix },
  { "<=", BP_CMP, NULL, infix },
  { "<", BP_CMP, NULL, infix },
  { "!=", BP_CMP, NULL, infix },
  { "<<", BP_BIT_SHIFT, NULL, infix },
  { ">>", BP_BIT_SHIFT, NULL, infix },
  { "<eof>", BP_NONE, NULL, NULL },
  { "<number>", BP_NONE, num, NULL },
  { "<name>", BP_NONE, variable, NULL },
  { "<string>", BP_NONE, string, NULL },
};

static expr * function_call (LexState *ls, expr * left) {
  expr * func_call = malloc(sizeof(expr));
  func_call -> type = EXPR_CALL;
  func_call -> call.callee = left;
  func_call->call.args = init_dynamic_expr(ls->B);

  if (checknext(ls, TK_RIGHT_PAREN)) return func_call;

  do {
    add_element(ls->B, func_call->call.args, parse_statement(ls, BP_LOWEST));
  } while(checknext(ls, TK_COMMA));

  checknext(ls, TK_RIGHT_PAREN);
  return func_call;
}

static expr * infix (LexState *ls, expr * left) {
  expr * temp = malloc(sizeof(expr));

  // Supporting a++, a--
  if (ls->pre.type == ls->t.type) {
    if (ls->t.type == TK_ADD || ls->t.type == TK_SUB) {
      temp -> type = EXPR_CHANGE;
      temp -> change.op = ls->pre.type;
      temp -> change.val = left;
      testnext(ls, ls->pre.type);
    } else {
      syntax_error(ls, "Just supporting ++ or -- operator.");
    }
  } else {
    temp -> type = EXPR_BINARY;
    temp -> infix.op = ls->pre.type;
    temp -> infix.left = left;
    temp -> infix.right = parse_statement(ls, symbol_table[ls->pre.type].lbp);
    if (temp -> infix.op == TK_LEFT_BRACKET) testnext(ls, TK_RIGHT_BRACKET);
  }

  return temp;
}

static expr * parse_branch(struct LexState *ls, bindpower rbp) {
  checknext(ls, TK_IF);
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

static void skip_semicolon(LexState * ls) { // Skip all the semicolon
  while (checknext(ls, TK_SEMI)) ;
}

static expr * parse_statement(struct LexState *ls, bindpower rbp) {
  skip_semicolon(ls);
  expr * tree;
  switch(ls->t.type) {
    case TK_BREAK:
      return parse_break(ls, rbp);
    case TK_WHILE:
      return parse_while(ls, rbp);
    case TK_IF:
      return parse_branch(ls, rbp);
    case TK_FUNCTION:
      return parse_definition(ls);
    case TK_GLOBAL: {
      beanX_next(ls);
      if (ls->t.type != TK_NAME) {
        syntax_error(ls, "Must provide a variable name.");
      }
    }
  }
  tree = symbol_table[ls->t.type].nud(ls, NULL);

  while (rbp < get_tk_precedence(ls)) {
    Token token = ls->t;
    beanX_next(ls);
    tree = symbol_table[token.type].led(ls, tree);
  }

  skip_semicolon(ls);
  return tree;
};


static TValue * parse_program(LexState * ls) {
  expr * ex = parse_statement(ls, BP_LOWEST);
  return eval(ls->B, ex);
}

void bparser(LexState * ls) {
  beanX_next(ls);

  while (ls -> current != EOZ) {
    parse_program(ls);
  }
}

void bparser_for_line(LexState * ls, TValue ** ret) {
  beanX_next(ls);

  *ret = parse_program(ls);
}
