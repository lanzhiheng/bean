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


typedef struct LexState LexState;

void bparser(struct LexState * ls);

#endif
