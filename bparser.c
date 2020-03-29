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

static uint8_t last_2_opcode(bean_State * B) {
  Mbuffer * buff = G(B)->instructionStream;
  return buff->buffer[buff->n - 2];
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
static void parse_statement(LexState *ls, bindpower rbp);
static void infix (LexState *ls);
static void function_call (LexState *ls);

static void string (LexState *ls) {
  write_byte(ls->B, OP_BEAN_PUSH_STR);
  operand_pointer_encode(ls->B, ls->t.seminfo.ts);
  beanX_next(ls);
}

static void num(LexState *ls) {
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
}

static void nil(LexState *ls) {
  write_opcode(ls->B, OP_BEAN_PUSH_NIL);
  beanX_next(ls);
}

static void boolean(LexState *ls) {
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
}

static void self(LexState *ls) {
  checknext(ls, TK_SELF);
  write_opcode(ls->B, OP_BEAN_SELF_GET);
}

static void left_paren(LexState *ls) {
  checknext(ls, TK_LEFT_PAREN);
  parse_statement(ls, BP_LOWEST);
  checknext(ls, TK_RIGHT_PAREN);
}

static void variable(LexState *ls) {
  Token token = ls->t;
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_PUSH_STR);
  operand_pointer_encode(ls->B, token.seminfo.ts);

  write_opcode(ls->B, OP_BEAN_VARIABLE_GET);
}

static void return_exp(LexState *ls) {
  beanX_next(ls);

  // TODO: support to return the null value
  if (ls -> t.type == TK_RIGHT_BRACE) syntax_error(ls, "You have to set the return value after the return statement");

  parse_statement(ls, BP_LOWEST);
  write_opcode(ls->B, OP_BEAN_RETURN); // Get return address from stack
}

static void parse_definition(LexState *ls) {
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
}

static void unary(LexState *ls) {
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
}

static void parse_array(struct LexState *ls UNUSED) {
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_ARRAY_PUSH);

  if (checknext(ls, TK_RIGHT_BRACKET)) return;

  do {
    parse_statement(ls, BP_LOWEST);
    write_opcode(ls->B, OP_BEAN_ARRAY_ITEM);
  } while(checknext(ls, TK_COMMA));
  checknext(ls, TK_RIGHT_BRACKET);
}

static void parse_hash(struct LexState *ls UNUSED) {
  beanX_next(ls);

  write_byte(ls->B, OP_BEAN_HASH_NEW);
  if (checknext(ls, TK_RIGHT_BRACE)) return;

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
};

static void function_call (LexState *ls) {
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
}

static void infix (LexState *ls) {
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
  } else if (binaryOp == TK_AND) {
    size_t before;
    write_opcode(ls->B, OP_BEAN_JUMP_FALSE_PUSH_BACK);
    before = G(ls->B) -> instructionStream -> n;
    write_init_offset(ls->B);
    parse_statement(ls, symbol_table[binaryOp].lbp);
    offset_patch(ls->B, before, G(ls->B) -> instructionStream -> n - before);
  } else if (binaryOp == TK_OR) {
    size_t before;
    write_opcode(ls->B, OP_BEAN_JUMP_TRUE_PUSH_BACK);
    before = G(ls->B) -> instructionStream -> n;
    write_init_offset(ls->B);
    parse_statement(ls, symbol_table[binaryOp].lbp);
    offset_patch(ls->B, before, G(ls->B) -> instructionStream -> n - before);
  } else {
    char patchOp = 0;

    if (binaryOp == TK_ASSIGN) { // If need assign, don't eval variable
      if (last_opcode(ls->B) == OP_BEAN_VARIABLE_GET) {
        delete_opcode(ls->B);
      } else if (last_2_opcode(ls->B) == OP_BEAN_BINARY_OP) {
        char lastOp = last_opcode(ls->B);
        if (lastOp == TK_DOT) { // a.b = xxx
          delete_opcode(ls->B);
          write_byte(ls->B, OP_BEAN_NEXT_STEP);
          patchOp = OP_BEAN_HASH_ATTRIBUTE_ASSIGN;
        } else if (lastOp == TK_LEFT_BRACKET) { // a[0] = xxx Or a['hello'] = xxx
          delete_opcode(ls->B);
          write_byte(ls->B, OP_BEAN_NEXT_STEP);
          patchOp = OP_BEAN_INDEX_OR_ATTRIBUTE_ASSIGN;
        }
      }
    }

    parse_statement(ls, symbol_table[binaryOp].lbp);
    if (binaryOp == TK_DOT) {
      delete_opcode(ls->B);
    }

    if (binaryOp == TK_LEFT_BRACKET) {
      testnext(ls, TK_RIGHT_BRACKET);
    }

    write_byte(ls->B, OP_BEAN_BINARY_OP);
    if (patchOp) {
      write_byte(ls->B, patchOp);
    } else {
      write_byte(ls->B, binaryOp);
    }
  }
}

static void parse_variable_definition(struct LexState *ls, bindpower rbp UNUSED) {
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
}

static void parse_branch(struct LexState *ls, bindpower rbp) {
  size_t offIf, offEndif;
  bool singleBranch = false;
  if (ls->t.type == TK_IF) {
    testnext(ls, TK_IF);
    singleBranch = true;
  }

  if (ls->t.type == TK_ELSEIF) testnext(ls, TK_ELSEIF);
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
    if (singleBranch) write_opcode(ls->B, OP_BEAN_PUSH_NIL);
  }
}

static void parse_do_while(struct LexState *ls, bindpower rbp) {
  size_t loopEnd, loopTop, loopPush;
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_PUSH_NIL);
  write_opcode(ls->B, OP_BEAN_NEW_SCOPE);
  write_opcode(ls->B, OP_BEAN_LOOP);
  loopPush = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);
  loopTop = G(ls->B)->instructionStream -> n;

  testnext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    write_opcode(ls->B, OP_BEAN_DROP);
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
}

static void parse_while(struct LexState *ls, bindpower rbp) {
  size_t loopBegin, loopEnd, loopTop, loopPush;
  beanX_next(ls);

  write_opcode(ls->B, OP_BEAN_PUSH_NIL);

  write_opcode(ls->B, OP_BEAN_NEW_SCOPE);
  write_opcode(ls->B, OP_BEAN_LOOP); // start loop
  loopPush = G(ls->B)->instructionStream -> n; // start address
  write_init_offset(ls->B);
  loopTop = G(ls->B)->instructionStream -> n;

  parse_statement(ls, rbp); // condition

  write_opcode(ls->B, OP_BEAN_JUMP_FALSE);

  loopBegin = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);

  testnext(ls, TK_LEFT_BRACE);

  while (ls->t.type != TK_RIGHT_BRACE) {
    write_opcode(ls->B, OP_BEAN_DROP);
    parse_statement(ls, rbp);
  }
  testnext(ls, TK_RIGHT_BRACE);

  write_opcode(ls->B, OP_BEAN_LOOP_CONTINUE);
  loopEnd = G(ls->B)->instructionStream -> n;
  write_init_offset(ls->B);
  offset_patch(ls->B, loopEnd, loopTop - loopEnd);

  loopEnd = G(ls->B)->instructionStream -> n;
  offset_patch(ls->B, loopBegin, loopEnd - loopBegin);

  write_opcode(ls->B, OP_BEAN_LOOP_BREAK);
  offset_patch(ls->B, loopPush, loopEnd); // End of loop's address.

  write_opcode(ls->B, OP_BEAN_END_SCOPE);
}

static void skip_semicolon(LexState * ls) { // Skip all the semicolon
  while (checknext(ls, TK_SEMI)) ;
}

static void parse_break(struct LexState *ls UNUSED, bindpower rbp UNUSED) {
  write_opcode(ls->B, OP_BEAN_LOOP_BREAK);

  beanX_next(ls);
  skip_semicolon(ls); // Skip all the semicolon
}

static void parse_statement(struct LexState *ls, bindpower rbp) {
  skip_semicolon(ls);

  switch(ls->t.type) {
    case TK_BREAK:
      parse_break(ls, rbp);
      return;
    case TK_WHILE:
      parse_while(ls, rbp);
      return;
    case TK_DO:
      parse_do_while(ls, rbp);
      return;
    case TK_IF:
      parse_branch(ls, rbp);
      return;
    case TK_FUNCTION:
      parse_definition(ls);
      return;
    case TK_VAR: {
      beanX_next(ls);
      if (ls->t.type != TK_NAME) {
        syntax_error(ls, "Must provide a variable name.");
      }
      parse_variable_definition(ls, rbp);
      return;
    }
  }
  symbol_table[ls->t.type].nud(ls);

  while (rbp < get_tk_precedence(ls)) {
    Token token = ls->t;
    beanX_next(ls);
    symbol_table[token.type].led(ls);
  }

  skip_semicolon(ls);
};


static TValue * parse_program(LexState * ls) {
  parse_statement(ls, BP_LOWEST);
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
