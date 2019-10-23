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
#define MAXSTRTB	cast_int(beanM_limitN(INT_MAX, TString*))


// Size of String is sum of the size of struct TString and the extra char array with the '\0' at the end of the string.
#define sizeofstring(s)  (sizeof(TString) + ((s) + 1) * sizeof(char))
#define isreserved(s)	((s)->tt == BEAN_TSTRING && (s)->reserved > 0)
#define getstr(ts) (cast_charp((ts)) + sizeof(TString))

#define beanS_newliteral(B, s) beanS_newlstr(B, "" s, strlen(s))

TString * beanS_newlstr (bean_State * B, const char *str, size_t l);

#endif
