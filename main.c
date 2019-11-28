#include "bstate.h"
#include "bparser.h"
#include "blex.h"
#include "bstring.h"
#include <stdio.h>
#include <sys/stat.h>

char * rootDir = NULL;

#define io_error(message) printf("%s", message)
#define mem_error(message) printf("%s", message)

char * read_source_file(const char * path) {
  FILE * file = fopen(path, "r");

  if (file == NULL) io_error("Can not open file");

  struct stat fileStat;
  stat(path, &fileStat);
  size_t fileSize = fileStat.st_size;

  printf("File size is: %lu.\n", fileSize);
  char * fileContent = (char *)malloc(fileSize + 1);
  if (fileContent == NULL) {
    mem_error("Could't allocate memory for reading file");
  }

  size_t numRead = fread(fileContent, sizeof(char), fileSize, file);

  if (numRead < fileSize) io_error("Could't read file");

  fileContent[fileSize] = '\0';
  fclose(file);
  return fileContent;
}

static void run_file(const char * path) {
  const char * lastSlash = strrchr(path, '/');
  const char * filename = path;

  if (lastSlash != NULL) {
    char * root = (char*)malloc(lastSlash - path + 2); // Allocate more char space for string '/' and '\0'
    memcpy(root, path, lastSlash - path + 1);
    root[lastSlash - path + 1] = '\0';
    rootDir = root;
    filename = lastSlash + 1;
  }

  bean_State * B = malloc(sizeof(B));
  LexState * ls = malloc(sizeof(LexState));
  B -> allocateBytes = 0;
  global_init(B);
  beanX_init(B);

  TString * e = beanS_newlstr(B, filename, strlen(filename));
  printf("Source file name is %s.\n", getstr(e));
  char* source = read_source_file(path);
  beanX_setinput(B, ls, source, e, *source);
  bparser(ls);
}

int main(int argc, char ** argv) {
  if (argc == 1) {
    ;
  } else {
    run_file(argv[1]);
  }
  return 0;
}
