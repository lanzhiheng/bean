#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

void hello(const char * c) {
  /* printf("%s\n", c); */
  /* printf("%lu", sizeof((c))); */
  printf("%lu", sizeof(size_t));
}

void b_str2int(const char * s, long result) {

}

int main() {
  char *ptr;
  printf("%lu\n", sizeof(size_t));
  errno = 0;
  char * org = ptr;
  printf("%ld\n", strtol("  100ddd  ", &ptr, 10));
  printf("%d\n", errno);
  printf("%c\n", *ptr);
  printf("%lu\n", sizeof(long));
}
