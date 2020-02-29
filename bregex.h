#ifndef BEAN_REGEX_H
#define BEAN_REGEX_H

#include "bstring.h"
#include "bobject.h"
#include "bstate.h"

TValue * init_regex(bean_State * B, char * matchStr, char * modeStr);
TValue * init_Regex(bean_State * B);
#endif
