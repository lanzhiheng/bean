#ifndef BEAN_STRING_H
#define BEAN_STRING_H

#include "bstate.h"
#include "bobject.h"

#define MEMERRMSG       "not enough memory"

// Size of String is sum of the size of struct TString and the extra char array with the '\0' at the end of the string.
#define sizelstring(l)  (sizeof(TString) + ((l) + 1) * sizeof(char))

TString * beanS_newString(bean_State * L, char * s);

#endif
