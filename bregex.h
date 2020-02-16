#ifndef BEAN_REGEX_H
#define BEAN_REGEX_H

#include "bstring.h"
#include "bobject.h"
#include "bstate.h"

Hash * init_regex(bean_State * B, TString * matchStr);
TValue * get_match(bean_State * B, TValue * this);
TValue * init_Regex(bean_State * B);
#endif
