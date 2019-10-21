#ifndef BEAN_LGC_H
#define BEAN_LGC_H

#include "common.h"
#include "bstate.h"

GCObject *beanC_newobj (bean_State *B, int tt, size_t sz);
#endif
