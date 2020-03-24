#include <limits.h>
#include <assert.h>
#include "bstring.h"
#include "vm.h"
#include "bzio.h"
#include "berror.h"
#include "bhash.h"
#include "bparser.h"

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

static size_t offset_patch(bean_State * B, size_t index, size_t offset) {
  Mbuffer * buff = G(B)->instructionStream;
  size_t llen = 0;

  while (llen < COMMON_POINTER_SIZE) {
    uint8_t b = offset & 0xff;
    buff->buffer[index++] = b;
    offset >>= 8;
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

static uint8_t last_opcode(bean_State * B) {
  Mbuffer * buff = G(B)->instructionStream;
  return buff->buffer[buff->n - 1];
}

static void delete_opcode(bean_State * B) {
  Mbuffer * buff = G(B)->instructionStream;
  buff->n--;
}

static void write_init_offset(bean_State * B) {
  size_t i;
  for (i = 0; i < COMMON_POINTER_SIZE; i++) {
    write_byte(B, 0);
  }
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
      bean_Number * value = malloc(sizeof(bean_Number));
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
  parse_statement(ls, BP_LOWEST);
  checknext(ls, TK_RIGHT_PAREN);
  return NULL;
}

static expr* variable(LexState *ls, expr *exp UNUSED) {
  Token token = ls->t;
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_PUSH_STR);
  operand_pointer_encode(ls->B, token.seminfo.ts);

  write_opcode(ls->B, OP_BEAN_VARIABLE_GET);

  return NULL;
}

static expr* return_exp(LexState *ls, expr * exp UNUSED) {
  beanX_next(ls);

  // TODO: support to return the null value
  if (ls -> t.type == TK_RIGHT_BRACE) syntax_error(ls, "You have to set the return value after the return statement");

  parse_statement(ls, BP_LOWEST);
  write_opcode(ls->B, OP_BEAN_RETURN); // Get return address from stack
  return NULL;
}

static expr * parse_definition(LexState *ls) {
  TokenType tkType = ls->pre.type;
  testnext(ls, TK_FUNCTION);

  size_t prologue, off;
  bool nameProvided = false;

  if (ls->t.type == TK_NAME) {
    nameProvided = true;
    write_opcode(ls->B, OP_BEAN_PUSH_STR);
    operand_pointer_encode(ls->B, ls->t.seminfo.ts);
    beanX_next(ls);
  }

  write_opcode(ls->B, OP_BEAN_JUMP);
  off = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);

  prologue = G(ls->B)->instructionStream -> n; // Function body start here.

  write_opcode(ls->B, OP_BEAN_NEW_FUNCTION_SCOPE);

  testnext(ls, TK_LEFT_PAREN);

  long args = 0;
  while (ls->t.type != TK_RIGHT_PAREN) {
    write_opcode(ls->B, OP_BEAN_SET_ARG);
    operand_pointer_encode(ls->B, (void *)args++);
    operand_pointer_encode(ls->B, ls->t.seminfo.ts);
    beanX_next(ls);
    if (ls->t.type == TK_COMMA) beanX_next(ls);
  }

  testnext(ls, TK_RIGHT_PAREN);

  checknext(ls, TK_LEFT_BRACE);

  write_opcode(ls->B, OP_BEAN_PUSH_NIL);
  while (ls->t.type != TK_RIGHT_BRACE) {
    write_opcode(ls->B, OP_BEAN_DROP);
    parse_statement(ls, BP_LOWEST);
  }

  testnext(ls, TK_RIGHT_BRACE);

  write_opcode(ls->B, OP_BEAN_RETURN); // Get return address from stack

  offset_patch(ls->B, off, G(ls->B)->instructionStream -> n - off); // jump here
  write_opcode(ls->B, OP_BEAN_FUNCTION_DEFINE);
  operand_pointer_encode(ls->B, (void *)prologue);

  if (nameProvided) {
    write_opcode(ls->B, OP_BEAN_SET_FUNCTION_NAME);
    write_byte(ls->B, tkType == TK_COLON || tkType == TK_ASSIGN); // Don't set name to scope
  }

  return NULL;
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

  write_opcode(ls->B, OP_BEAN_ARRAY_PUSH);

  if (checknext(ls, TK_RIGHT_BRACKET)) return NULL;

  do {
    parse_statement(ls, BP_LOWEST);
    write_opcode(ls->B, OP_BEAN_ARRAY_ITEM);
  } while(checknext(ls, TK_COMMA));
  checknext(ls, TK_RIGHT_BRACKET);
  return NULL;
}

static expr * parse_hash(struct LexState *ls UNUSED, expr * exp UNUSED) {
  beanX_next(ls);

  write_byte(ls->B, OP_BEAN_HASH_NEW);
  if (checknext(ls, TK_RIGHT_BRACE)) return NULL;

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
    parse_statement(ls, BP_LOWEST);
    write_byte(ls->B, OP_BEAN_HASH_VALUE);
  } while(checknext(ls, TK_COMMA));

  checknext(ls, TK_RIGHT_BRACE);
  return NULL;
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
  int args = 0;
  size_t off;

  checknext(ls, TK_LEFT_PAREN);
  write_opcode(ls->B, OP_BEAN_CREATE_FRAME);
  off = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);

  if (checknext(ls, TK_RIGHT_PAREN)) {

  } else {
    do {
      args++;
      parse_statement(ls, BP_LOWEST);
      write_opcode(ls->B, OP_BEAN_IN_STACK);
    } while(checknext(ls, TK_COMMA));
    if (args > MAX_ARGS) syntax_error(ls, "SyntaxError: The arguments you are passing up max size of args.");
    testnext(ls, TK_RIGHT_PAREN);
  }

  write_opcode(ls->B, OP_BEAN_FUNCTION_CALL0 + args);
  offset_patch(ls->B, off, G(ls->B)->instructionStream -> n);

  return NULL;
}

static expr * infix (LexState *ls, expr * left) {
  int binaryOp = ls->pre.type;

  if (ls->t.type == TK_ASSIGN) {
    if (binaryOp >= TK_ADD && binaryOp <= TK_MOD) {
      testnext(ls, TK_ASSIGN);
      if (last_opcode(ls->B) == OP_BEAN_VARIABLE_GET) {
        delete_opcode(ls->B);
      } else {
        syntax_error(ls, "SyntaxError: This operation just support for variable.");
      }

      parse_statement(ls, symbol_table[binaryOp].lbp);
      write_opcode(ls->B, OP_BEAN_BINARY_OP_WITH_ASSIGN);
      write_byte(ls->B, binaryOp);
    } else {
      syntax_error(ls, "Just supporting +=, -=, *=, /=, |=, &=, %=, ^=");
    }
  } else if (ls->t.type == binaryOp) {
    if (ls->t.type == TK_ADD || ls->t.type == TK_SUB) {
      if (last_opcode(ls->B) == OP_BEAN_VARIABLE_GET) {
        delete_opcode(ls->B);
      } else {
        syntax_error(ls, "SyntaxError: This operation just support for variable.");
      }

      write_opcode(ls->B, OP_BEAN_HANDLE_SUFFIX);
      write_byte(ls->B, binaryOp);
      beanX_next(ls);
    } else {
      syntax_error(ls, "Just supporting a++ and a--");
    }
  } else {
    if (binaryOp == TK_ASSIGN) {
      delete_opcode(ls->B);
    }

    parse_statement(ls, symbol_table[binaryOp].lbp);
    if (binaryOp == TK_DOT) {
      delete_opcode(ls->B);
    }

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
  size_t offIf, offEndif;

  if (ls->t.type == TK_IF || ls->t.type == TK_ELSEIF) beanX_next(ls);
  write_opcode(ls->B, OP_BEAN_NEW_SCOPE);

  testnext(ls, TK_LEFT_PAREN);
  parse_statement(ls, rbp); // condition
  testnext(ls, TK_RIGHT_PAREN);

  write_opcode(ls->B, OP_BEAN_JUMP_FALSE);
  offIf = G(ls->B)->instructionStream->n;

  write_init_offset(ls->B); // placeholder

  testnext(ls, TK_LEFT_BRACE);

  write_opcode(ls->B, OP_BEAN_PUSH_NIL);
  while (ls->t.type != TK_RIGHT_BRACE) {
    write_opcode(ls->B, OP_BEAN_DROP);
    parse_statement(ls, rbp);
  }
  testnext(ls, TK_RIGHT_BRACE);

  if (ls->t.type == TK_ELSEIF) {
    size_t offElse, offEndelse;
    write_opcode(ls->B, OP_BEAN_JUMP);
    offElse = G(ls->B)->instructionStream->n;
    write_init_offset(ls->B); // placeholder
    offEndif = G(ls->B)->instructionStream->n;
    offset_patch(ls->B, offIf, offEndif - offIf);

    parse_branch(ls, rbp);

    offEndelse = G(ls->B)->instructionStream->n;
    offset_patch(ls->B, offElse, offEndelse - offElse);
  } else if (ls->t.type == TK_ELSE) {

    size_t offElse, offEndelse;

    testnext(ls, TK_ELSE);
    write_opcode(ls->B, OP_BEAN_NEW_SCOPE);
    write_opcode(ls->B, OP_BEAN_JUMP);
    offElse = G(ls->B)->instructionStream->n;
    write_init_offset(ls->B); // placeholder
    offEndif = G(ls->B)->instructionStream->n;
    offset_patch(ls->B, offIf, offEndif - offIf);

    testnext(ls, TK_LEFT_BRACE);

    write_opcode(ls->B, OP_BEAN_PUSH_NIL);
    while (ls->t.type != TK_RIGHT_BRACE) {
      write_opcode(ls->B, OP_BEAN_DROP);
      parse_statement(ls, rbp);
    }
    testnext(ls, TK_RIGHT_BRACE);

    offEndelse = G(ls->B)->instructionStream->n;
    offset_patch(ls->B, offElse, offEndelse - offElse);
    write_opcode(ls->B, OP_BEAN_END_SCOPE);
  } else {
    offEndif = G(ls->B)->instructionStream->n;
    offset_patch(ls->B, offIf, offEndif - offIf);
    write_opcode(ls->B, OP_BEAN_END_SCOPE);
  }

  return NULL;
}

static expr * parse_do_while(struct LexState *ls, bindpower rbp) {
  size_t loopEnd, loopTop, loopPush;
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_LOOP);
  loopPush = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);
  loopTop = G(ls->B)->instructionStream -> n;

  write_opcode(ls->B, OP_BEAN_NEW_SCOPE);

  testnext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    parse_statement(ls, rbp);
  }

  testnext(ls, TK_RIGHT_BRACE);

  testnext(ls, TK_WHILE);

  parse_statement(ls, rbp);

  write_opcode(ls->B, OP_BEAN_JUMP_TRUE);
  loopEnd = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);
  offset_patch(ls->B, loopEnd, loopTop - loopEnd);

  write_opcode(ls->B, OP_BEAN_LOOP_BREAK);
  offset_patch(ls->B, loopPush, G(ls->B)->instructionStream -> n); // End of loop's address.
  write_opcode(ls->B, OP_BEAN_END_SCOPE);

  return NULL;
}

static expr * parse_while(struct LexState *ls, bindpower rbp) {
  size_t loopBegin, loopEnd, loopTop, loopPush;
  beanX_next(ls);
  write_opcode(ls->B, OP_BEAN_LOOP);
  loopPush = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);
  loopTop = G(ls->B)->instructionStream -> n;

  parse_statement(ls, rbp); // condition

  write_opcode(ls->B, OP_BEAN_JUMP_FALSE);
  loopBegin = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);

  write_opcode(ls->B, OP_BEAN_NEW_SCOPE);

  testnext(ls, TK_LEFT_BRACE);
  while (ls->t.type != TK_RIGHT_BRACE) {
    parse_statement(ls, rbp);
  }
  testnext(ls, TK_RIGHT_BRACE);

  write_opcode(ls->B, OP_BEAN_LOOP_CONTINUE);
  loopEnd = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);
  offset_patch(ls->B, loopEnd, loopTop - loopEnd);

  loopEnd = G(ls->B)->instructionStream -> n;
  offset_patch(ls->B, loopBegin, loopEnd - loopBegin);

  offset_patch(ls->B, loopPush, loopEnd); // End of loop's address.
  write_opcode(ls->B, OP_BEAN_LOOP_BREAK);

  write_opcode(ls->B, OP_BEAN_END_SCOPE);
  return NULL;
}

static void skip_semicolon(LexState * ls) { // Skip all the semicolon
  while (checknext(ls, TK_SEMI)) ;
}

static expr * parse_break(struct LexState *ls UNUSED, bindpower rbp UNUSED) {
  write_opcode(ls->B, OP_BEAN_LOOP_BREAK);

  beanX_next(ls);
  skip_semicolon(ls); // Skip all the semicolon
  return NULL;
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
  }
}

void bparser_for_line(LexState * ls, TValue ** ret) {
  beanX_next(ls);

  *ret = parse_program(ls);
}
