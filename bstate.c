#include "bstate.h"
#include "mem.h"

void global_init(bean_State * B) {
  global_State * G = malloc(sizeof(global_State));
  G -> seed = rand();
  G -> allgc = NULL;
  stringtable tb;
  tb.nuse = 0;
  tb.size = 10;
  tb.hash = beanM_reallocvchar(B, NULL, 0, 10);

  for (int i = 0; i < 10; i ++) {
    tb.hash[i] = NULL;
  }
  G -> strt = tb;
  B -> l_G = G;
}
