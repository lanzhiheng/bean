#ifndef _BEAN_PARSER_H
#define _BEAN_PARSER_H

#include "common.h"
#include "vm.h"

typedef enum {
    // Data type
    TOKEN_UNKNOWN,
    TOKEN_NUM, // number
    TOKEN_STRING, // string
    TOKEN_ID, // variable name
    TOKEN_INTERPOLATION, // embedded expression

    // Keyword
    TOKEN_VAR, // var
    TOKEN_FUN, // fun
    TOKEN_IF, // if
    TOKEN_ELSE, // else
    TOKEN_TRUE, // true
    TOKEN_FALSE, // false
    TOKEN_WHILE, // while
    TOKEN_FOR, // for
    TOKEN_BREAK, // break
    TOKEN_CONTINUE, // continue
    TOKEN_RETURN, // return
    TOKEN_NIL, // nil

    // class/module import
    TOKEN_THIS, // this
    TOKEN_IMPORT, // import

    // delimiter
    TOKEN_COMMA, // ,
    TOKEN_COLON, // ;
    TOKEN_LEFT_PAREN, // (
    TOKEN_RIGHT_PAREN, // )
    TOKEN_LEFT_BRACKET, // [
    TOKEN_RIGHT_BRACKET, // ]
    TOKEN_LEFT_BRACE, // {
    TOKEN_RIGHT_BRACE, //
    TOKEN_DOT, // .
    TOKEN_DOT_DOT, // ..

    // Binary arithmetic operators
    TOKEN_ADD, // +
    TOKEN_SUB, // -
    TOKEN_MUL, // *
    TOKEN_DIV, // /
    TOKEN_MOD, // %

    // assign
    TOKEN_ASSIGN, // =

    // bit operators
    TOKEN_BIT_ADD, // &
    TOKEN_BIT_OR, // |
    TOKEN_BIT_NOT, // ~
    TOKEN_BIT_SHIFT_RIGHT, // >>
    TOKEN_BIT_SHIFT_LEFT, // <<

    // logical operators
    TOKEN_LOGIC_AND, // &&
    TOKEN_LOGIC_OR, // ||
    TOKEN_LOGIC_NOT, // !

    // comparison operator
    TOKEN_EQUAL, // ==
    TOKEN_NOT_EQUAL, // !=
    TOKEN_GREATER, // >
    TOKEN_GREATER_EQUAL, // >=
    TOKEN_LESS, // <
    TOKEN_LESS_EQUAL, // <=

    TOKEN_QUESTION, // ?

    // End of the file
    TOKEN_EOF // EOF
} TokenType;

typedef struct {
  TokenType type;
  const char* start; // Token的启始地址
  uint32_t length; // Token的长度
  uint32_t lineNo; // Token的行号
  Value value; // Token的值
} Token;

struct parser {
  const char * file;
  const char * sourceCode;
  const char * nextCharPtr;
  char curChar;
  Token curToken;
  Token preToken;
  /* Module * curModule; */
  /* CompileUnit * curCompileUnit;  // 当前编译单元 */
  int interpolationExpectRightParenNum;
  struct parser * parent; // 指向父parser
  VM * vm;
};

#define PEEK_TOKEN(parserPtr) parserPtr -> curToken.type

char lookAheadChar(Parser * parser);
void getNextToken (Parser * parser);
bool matchToken (Parser * parser, TokenType expected);
void consumeCurToken (Parser * parser, TokenType expected, const char * errMsg);
void consumeNextToken (Parser * parser, TokenType expected, const char * errMsg);
/* void initParser (VM* vm, Parser* parser, const char* file, const char* sourceCode, Module * objModule); */

#endif
