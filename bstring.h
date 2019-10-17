#ifndef BEAN_STRING_H
#define BEAN_STRING_H

#include "bstate.h"
#include "bobject.h"
#include "blex.h"

#define MEMERRMSG       "not enough memory"

// Size of String is sum of the size of struct TString and the extra char array with the '\0' at the end of the string.
#define sizelstring(l)  (sizeof(TString) + ((l) + 1) * sizeof(char))

#define isreserved(ts) true // TODO: Need to add the string func

TString * beanS_newliteral(bean_State * B, char * s);
TString * beanX_newstring(LexState * ls, char * s, size_t l);

#endif
