#ifndef BEAN_LEX_H
#define BEAN_LEX_H
#include "commen.h"

#define FIRST_RESERVED	257

/* #if !defined(BEAN_ENV) */
/* #define BEAN_ENV		"_ENV" */
/* #endif */

struct lua_State {
  lua_State
}

typedef enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_AND = FIRST_RESERVED, TK_BREAK,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
  /* other terminal symbols */
  TK_IDIV, TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE,
  TK_SHL, TK_SHR,
  TK_DBCOLON, TK_EOS,
  TK_FLT, TK_INT, TK_NAME, TK_STRING
} TokenType;

#define NUM_RESERVED	(cast_int(TK_WHILE-FIRST_RESERVED + 1))

typedef union {
  lua_Number r;
  lua_Integer i;
  TString *ts;
} SemInfo;  /* semantics information */












typedef struct Token {
  TokenType token;
  SemInfo seminfo;
} Token;

typedef struct LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token 'consumed' */
  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  /* struct FuncState *fs;  /\* current function (parser) *\/ */
  /* struct lua_State *L; */
  char * inputStream;
  TString *source;  /* current source name */
  TString *envn;  /* environment variable name */
} LexState

static void beanX_init (lua_State *L);

#endif
