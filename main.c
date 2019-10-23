#include "stdio.h"
#include "bstate.h"
#include "blex.h"

int main() {
  bean_State B;
  B.allocateBytes = 0;
  global_init(&B);
  beanX_init(&B);
  return 0;
}
