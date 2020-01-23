#ifndef BEAN_PARSER_H
#define BEAN_PARSER_H

#include "blex.h"
#include "bobject.h"
#include "bhash.h"

#define MAX_LEN_ID 255

/*
** Marks the end of a patch list. It is an invalid value both as an absolute
** address, and as a list link (would link an element to itself).
*/
#define NO_JUMP (-1)


typedef struct LocalVar {
  TString * name;
  TValue value;
} LocalVar;

// Power
typedef enum {
  BP_NONE,
  BP_LOWEST,
  BP_ASSIGN,
  BP_CONDITION,
  BP_LOGIC_OR,
  BP_LOGIC_AND,
  BP_EQUAL,
  BP_IS,
  BP_CMP,
  BP_BIT_OR,
  BP_BIT_XOR,
  BP_BIT_AND,
  BP_BIT_SHIFT,
  BP_RANGE,
  BP_TERM,
  BP_FACTOR,
  BP_UNARY,
  BP_CALL,
  BP_DOT,
  BP_HIGHTEST
} bindpower;

typedef struct LexState LexState;
typedef expr* (*denotation_fn) (LexState *ls, expr * exp);

typedef struct symbol {
  char * id;
  bindpower lbp;
  denotation_fn nud;
  denotation_fn led;
} symbol;

extern symbol symbol_table[];

void bparser(LexState * ls);
void bparser_for_line(LexState * ls, TValue ** ret);

#endif
