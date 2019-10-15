#ifndef _BEAN_STATE_H
#define _BEAN_STATE_H

#include "common.h"

typedef struct bean_State {
  size_t allocateBytes;
} bean_State;

typedef struct FuncState {

} FuncState;

#include <assert.h>
#define bean_assert(c)           assert(c)

#endif
