#ifndef BEAN_VM_H
#define BEAN_VM_H
#include "bparser.h"

#define add(a, b) (a + b)
#define sub(a, b) (a - b)
#define mul(a, b) (a * b)
#define div(a, b) (a / b)

TValue * eval(bean_State * B, struct expr * expression);

#endif
