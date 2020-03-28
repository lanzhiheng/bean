#ifndef BEAN_FUNCTION_H
#define BEAN_FUNCTION_H

#include "bobject.h"
#include "bstate.h"
TValue * primitive_Function_call(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc);

TValue * primitive_Function_apply(bean_State * B, TValue * thisVal UNUSED, TValue * args, int argc);

TValue * init_Function(bean_State * B);
#endif
