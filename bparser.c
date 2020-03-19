#include <limits.h>
#include <assert.h>
#include "bstring.h"
#include "vm.h"
#include "bzio.h"
#include "berror.h"
#include "bhash.h"
#include "bparser.h"

#define MAX_ARGS 16 // MAX args of function
#define MIN_EXPR_SIZE 16
#define get_tk_precedence(ls) (symbol_table[ls->t.type].lbp)

static size_t operand_pointer_encode(bean_State * B, void * num) {
  Mbuffer * buff = G(B)->instructionStream;
  size_t llen = 0;
  int64_t pointer = (int64_t)num;

  while (llen < COMMON_POINTER_SIZE) {
    uint8_t b = pointer & 0xff;
    beanZ_append(B, buff, b);
    pointer >>= 8;
    llen++;
  }

  return llen;
}

static void write_byte(bean_State * B, uint8_t b) {
  Mbuffer * buff = G(B)->instructionStream;
  beanZ_append(B, buff, b);
}

static void write_opcode(bean_State * B, OpCode code) {
  write_byte(B, code);
}

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
  write_byte(ls->B, OP_BEAN_PUSH_STR);
  operand_pointer_encode(ls->B, ls->t.seminfo.ts);
  beanX_next(ls);
  return NULL;
}

static expr * regex(LexState * ls, expr * exp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_REGEX;
  ep -> regex.match = ls->t.seminfo.ts;
  ep -> regex.flag = 0;
  beanX_next(ls);
  return ep;
}

static expr* num(LexState *ls, expr * exp UNUSED) {
  switch(ls->t.type) {
    case(TK_NUM): {
      write_opcode(ls->B, OP_BEAN_PUSH_NUM);
      bean_Number * value = malloc(sizeof(double));
      *value = ls->t.seminfo.n;
      operand_pointer_encode(ls->B, value);
      break;
    }
    default:
      syntax_error(ls, "Not the valid number.");
  }
  beanX_next(ls);
  return NULL;
}

static expr* nil(LexState *ls, expr * exp UNUSED) {
  write_opcode(ls->B, OP_BEAN_PUSH_NIL);
  beanX_next(ls);
  return NULL;
}

static expr* boolean(LexState *ls, expr * exp UNUSED) {
  switch(ls->t.type) {
    case(TK_TRUE): {
      write_opcode(ls->B, OP_BEAN_PUSH_TRUE);
      break;
    }
    case(TK_FALSE): {
      write_opcode(ls->B, OP_BEAN_PUSH_FALSE);
      break;
    }
    default:
      syntax_error(ls, "Not the valid boolean value.");
  }
  beanX_next(ls);
  return NULL;
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
  Token token = ls->t;
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_PUSH_STR);
  operand_pointer_encode(ls->B, token.seminfo.ts);

  TokenType op = ls->t.type;
  // a++ or a--
  if (op >= TK_ADD && op <= TK_MOD) {
  } else {
    write_opcode(ls->B, OP_BEAN_VARIABLE_GET);
  }

  return NULL;
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
  checknext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    add_element(ls->B, f->body, parse_statement(ls, BP_LOWEST));
  }

  testnext(ls, TK_RIGHT_BRACE);
  expr * ex = malloc(sizeof(expr));
  ex->type = EXPR_FUN;
  ex->fun = f;

  write_opcode(ls->B, OP_BEAN_FUNCTION_DEFINE);
  operand_pointer_encode(ls->B, f);
  return ex;
}

static expr * unary(LexState *ls, expr * exp UNUSED) {
  expr * temp = malloc(sizeof(expr));

  TokenType type = ls->t.type;
  beanX_next(ls);

  // Supporting ++a, --a
  if (ls->pre.type == ls->t.type) {
    TokenType op = ls->t.type;

    if (ls->t.type == TK_ADD || ls->t.type == TK_SUB) {
      write_opcode(ls->B, OP_BEAN_HANDLE_PREFIX);
      write_byte(ls->B, op);
      testnext(ls, op);

      if (ls->t.type == TK_NAME) {
        Token token = ls->t;
        operand_pointer_encode(ls->B, token.seminfo.ts);
        beanX_next(ls);
      } else {
        syntax_error(ls, "++ or -- must follow a variable.");
      }
    } else {
      syntax_error(ls, "Just supporting ++ or -- operator.");
    }
  } else {
    parse_statement(ls, BP_LOWEST);
    write_opcode(ls->B, OP_BEAN_UNARY);
    write_byte(ls->B, type);
  }
  return temp;
}

static expr * parse_array(struct LexState *ls UNUSED, expr * exp UNUSED) {
  beanX_next(ls);
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_ARRAY;
  ep -> array = init_dynamic_expr(ls->B);

  write_opcode(ls->B, OP_BEAN_ARRAY_PUSH);

  if (checknext(ls, TK_RIGHT_BRACKET)) return ep;

  do {
    add_element(ls->B, ep->array, parse_statement(ls, BP_LOWEST));
    write_opcode(ls->B, OP_BEAN_ARRAY_ITEM);
  } while(checknext(ls, TK_COMMA));
  checknext(ls, TK_RIGHT_BRACKET);
  return ep;
}

static expr * parse_hash(struct LexState *ls UNUSED, expr * exp UNUSED) {
  beanX_next(ls);
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_HASH;
  ep -> hash = init_dynamic_expr(ls->B);

  write_byte(ls->B, OP_BEAN_HASH_NEW);
  if (checknext(ls, TK_RIGHT_BRACE)) return ep;

  do {
    Token token = ls->t;
    if (token.type == TK_NAME || token.type == TK_STRING) {
      write_opcode(ls->B, OP_BEAN_PUSH_STR);
      operand_pointer_encode(ls->B, token.seminfo.ts);
      beanX_next(ls);
    } else {
      syntax_error(ls, "Expect string or identifier as key.");
    }

    checknext(ls, TK_COLON);
    add_element(ls->B, ep->hash, parse_statement(ls, BP_LOWEST));
    write_byte(ls->B, OP_BEAN_HASH_VALUE);
  } while(checknext(ls, TK_COMMA));

  checknext(ls, TK_RIGHT_BRACE);
  return ep;
}

symbol symbol_table[] = {
  /* arithmetic operators */
  { "and", BP_LOGIC_AND, NULL, infix },
  { "break", BP_NONE, NULL, NULL },
  { "else", BP_NONE, NULL, NULL },
  { "elsif", BP_NONE, NULL, NULL },
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
  { "var", BP_NONE, NULL, NULL },
  { "nil", BP_NONE, nil, NULL },
  { "not", BP_NONE, unary, NULL },
  { "or", BP_LOGIC_OR, NULL, infix },
  { "return", BP_NONE, return_exp, NULL },
  { "true", BP_NONE, boolean, NULL },
  { "do", BP_NONE, NULL, NULL },
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
  { "<regex>", BP_NONE, regex, NULL },
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

  testnext(ls, TK_RIGHT_PAREN);
  return func_call;
}

static expr * infix (LexState *ls, expr * left) {
  int binaryOp = ls->pre.type;

  if (ls->t.type == TK_ASSIGN) {
    if (binaryOp >= TK_ADD && binaryOp <= TK_MOD) {
      testnext(ls, TK_ASSIGN);
      parse_statement(ls, symbol_table[binaryOp].lbp);
      write_byte(ls->B, OP_BEAN_BINARY_OP_WITH_ASSIGN);
      write_byte(ls->B, binaryOp);
    } else {
      syntax_error(ls, "Just supporting +=, -=, *=, /=, |=, &=, %=, ^=");
    }
  } else if (ls->t.type == binaryOp) {
    if (ls->t.type == TK_ADD || ls->t.type == TK_SUB) {
      write_opcode(ls->B, OP_BEAN_HANDLE_SUFFIX);
      write_byte(ls->B, binaryOp);
      beanX_next(ls);
    } else {
      syntax_error(ls, "Just supporting a++ and a--");
    }
  } else {
    parse_statement(ls, symbol_table[binaryOp].lbp);
    write_byte(ls->B, OP_BEAN_BINARY_OP);
    write_byte(ls->B, binaryOp);
  }

  return NULL;
}

static expr * parse_variable_definition(struct LexState *ls, bindpower rbp UNUSED) {
  Token token = ls->t;
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_PUSH_STR);
  operand_pointer_encode(ls->B, token.seminfo.ts);
  if (ls->t.type == TK_ASSIGN) {
    beanX_next(ls);
    parse_statement(ls, BP_LOWEST);
  } else { // Supporting multi variables
    write_opcode(ls->B, OP_BEAN_PUSH_NIL);
  }
  write_opcode(ls->B, OP_BEAN_VARIABLE_DEFINE);
  return NULL;
}

static expr * parse_branch(struct LexState *ls, bindpower rbp) {
  if (ls->t.type == TK_IF || ls->t.type == TK_ELSEIF) beanX_next(ls);
  expr * tree = malloc(sizeof(expr));
  tree -> type = EXPR_BRANCH;
  tree -> branch.condition = parse_statement(ls, rbp);
  tree -> branch.if_body = init_dynamic_expr(ls->B);

  testnext(ls, TK_LEFT_BRACE);
  while (ls->t.type != TK_RIGHT_BRACE) {
    add_element(ls->B, tree -> branch.if_body, parse_statement(ls, rbp));
  }
  testnext(ls, TK_RIGHT_BRACE);

  if (ls->t.type == TK_ELSEIF) { // elsif() {}  => else { if()  }
    tree -> branch.else_body = init_dynamic_expr(ls->B);
    add_element(ls->B, tree -> branch.else_body, parse_branch(ls, rbp));
  } else if (ls->t.type == TK_ELSE) {
    checknext(ls, TK_ELSE);
    tree -> branch.else_body = init_dynamic_expr(ls->B);
    testnext(ls, TK_LEFT_BRACE);
    while (ls->t.type != TK_RIGHT_BRACE) {
      add_element(ls->B, tree -> branch.else_body, parse_statement(ls, rbp));
    }
    testnext(ls, TK_RIGHT_BRACE);
  } else {
    tree -> branch.else_body = NULL;
  }
  return tree;
}

static expr * parse_do_while(struct LexState *ls, bindpower rbp) {
  beanX_next(ls);

  expr * tree = malloc(sizeof(expr));
  tree -> type = EXPR_LOOP;
  tree -> loop.body = init_dynamic_expr(ls->B);
  tree -> loop.firstcheck = false;
  testnext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    add_element(ls->B, tree->loop.body, parse_statement(ls, rbp));
  }

  testnext(ls, TK_RIGHT_BRACE);
  testnext(ls, TK_WHILE);
  tree -> loop.condition = parse_statement(ls, rbp);
  return tree;
}

static expr * parse_while(struct LexState *ls, bindpower rbp) {
  beanX_next(ls);

  expr * tree = malloc(sizeof(expr));
  tree -> type = EXPR_LOOP;
  tree -> loop.firstcheck = true;
  tree -> loop.condition = parse_statement(ls, rbp);
  tree -> loop.body = init_dynamic_expr(ls->B);
  testnext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    add_element(ls->B, tree->loop.body, parse_statement(ls, rbp));
  }

  testnext(ls, TK_RIGHT_BRACE);
  return tree;
}


static void skip_semicolon(LexState * ls) { // Skip all the semicolon
  while (checknext(ls, TK_SEMI)) ;
}

static expr * parse_break(struct LexState *ls UNUSED, bindpower rbp UNUSED) {
  expr * ep = malloc(sizeof(expr));
  ep -> type = EXPR_BREAK;
  beanX_next(ls);
  skip_semicolon(ls); // Skip all the semicolon
  return ep;
}

static expr * parse_statement(struct LexState *ls, bindpower rbp) {
  skip_semicolon(ls);
  expr * tree;

  switch(ls->t.type) {
    case TK_BREAK:
      return parse_break(ls, rbp);
    case TK_WHILE:
      return parse_while(ls, rbp);
    case TK_DO:
      return parse_do_while(ls, rbp);
    case TK_IF:
      return parse_branch(ls, rbp);
    case TK_FUNCTION:
      return parse_definition(ls);
    case TK_VAR: {
      beanX_next(ls);
      if (ls->t.type != TK_NAME) {
        syntax_error(ls, "Must provide a variable name.");
      }
      return parse_variable_definition(ls, rbp);
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
    write_opcode(ls->B, OP_BEAN_INSPECT);
  }
}

void bparser_for_line(LexState * ls, TValue ** ret) {
  beanX_next(ls);

  *ret = parse_program(ls);
}
