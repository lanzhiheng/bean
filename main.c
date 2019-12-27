#include "bstate.h"
#include "btests.h"

#ifdef TEST
int main(int argc, char ** argv) {
  test();
}
#else
int main(int argc, char ** argv) {
  if (argc == 1) {
    run();
  } else {
    run_file(argv[1]);
  }
  return 0;
}
#endif
