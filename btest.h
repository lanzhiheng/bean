#ifndef BEAN_TEST_H
#define BEAN_TEST_H

struct B_EXTRA { int lock; int *plock; };

#undef BEAN_EXTRASPACE
#define BEAN_EXTRASPACE	sizeof(struct B_EXTRA)
#endif
