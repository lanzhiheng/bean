#ifndef BEAN_STRING_H
#define BEAN_STRING_H

#include <string.h>
#include <errno.h>
#include "bstate.h"
#include "bobject.h"
#include "blex.h"
#include "lgc.h"

#define MEMERRMSG       "not enough memory"
#define HASHLIMIT		5

// Size of String is sum of the size of struct TString and the extra char array with the '\0' at the end of the string.
#define sizeofstring(s)  (sizeof(TString) + ((s) + 1) * sizeof(char))
#define isreserved(ts) true // TODO: Need to add the string func
#define getstr(ts) (cast_charp((ts)) + sizeof(TString))

TString * beanS_newliteral(bean_State * B, char * s);
TString * beanS_newlstr (bean_State * B, const char *str, size_t l);

#endif
