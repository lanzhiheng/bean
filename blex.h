#ifndef BEAN_LEX_H
#define BEAN_LEX_H
#include "stdio.h"
#include "common.h"
#include "bobject.h"
#include "bzio.h"
#include "bdebug.h"

#include <ctype.h>

// ref: https://www.programiz.com/c-programming/library-function/ctype.h
#define bislalpha(c)	(isalpha(c) || (c) == '_')
#define bislalnum(c)	(isalnum(c) || (c) == '_')
#define bisdigit(c)	(isdigit(c))
#define bisspace(c)	(isspace(c))
#define bisprint(c)	(isprint(c))
#define bisxdigit(c)	(isxdigit(c))

#define EOZ	('\0')			/* end of stream */

#define next(ls) (ls -> current = *(++ls -> inputStream))

#define currIsNewline(ls)	(ls->current == '\n' || ls->current == '\r')

#if !defined(BEAN_ENV)
#define BEAN_ENV		"BEAN_ENV"
#endif

typedef enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_AND, TK_BREAK, TK_ELSE, TK_ELSEIF,
  TK_ADD, TK_SUB,
  TK_MUL, TK_DIV,
  TK_COLON, TK_COMMA, TK_SEMI,
  TK_LEFT_BRACE, TK_RIGHT_BRACE,
  TK_LEFT_PAREN, TK_RIGHT_PAREN,
  TK_LEFT_BRACKET, TK_RIGHT_BRACKET, TK_DOT,
  TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_IF, TK_SELF, TK_IN, TK_VAR, TK_NIL, TK_NOT, TK_OR,
  TK_RETURN, TK_TRUE, TK_WHILE,
  /* other terminal symbols */
  TK_EQ, TK_ASSIGN, TK_GE, TK_GT, TK_LE, TK_LT, TK_NE,
  TK_SHL, TK_SHR,
  TK_EOS,
  TK_FLT, TK_INT, TK_NAME, TK_STRING
} TokenType;

/* ORDER RESERVED */
extern const char *const beanX_tokens[];


#define NUM_RESERVED	(cast_int(TK_WHILE + 1))

typedef union {
  bean_Number r;
  bean_Integer i;
  TString *ts;
} SemInfo;  /* semantics information */


typedef struct Token {
  int type;
  SemInfo seminfo;
} Token;

typedef struct LexState {
  int current;  /* current character (charint) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token 'consumed' */
  Token t;  /* current token */
  Token pre;  /* prev token */
  struct FuncState *fs;  /* current function (parser) */
  struct bean_State *B;
  Mbuffer *buff;  /* buffer for tokens */
  char * inputStream;
  struct Dyndata *dyd;
  TString *source;  /* current source name */
  TString *envn;  /* environment variable name */
} LexState;

void beanX_init (bean_State *B);
void beanX_setinput (bean_State *B, LexState *ls, char * inputStream, TString *source, int firstchar);
void beanX_next (LexState *ls);
const char *beanX_token2str (LexState *ls, int token);
const char *txtToken (LexState *ls, int token);
void beanX_syntaxerror (LexState *ls, const char *msg);
#endif
