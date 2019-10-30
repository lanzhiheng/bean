#ifndef _BEAN_DEBUG_H
#define _BEAN_DEBUG_H

#include "bstate.h"

const char * luaG_addinfo(bean_State * B, const char * msg, TString * source, int line);

#endif
