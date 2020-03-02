#ifndef BEAN_HTTP_H
#define BEAN_HTTP_H

#include <curl/curl.h>
#include "bstate.h"

TValue * init_Http(bean_State * B);
void fetch(char * url, char * method, char ** result);
TValue * init_Http(bean_State * B);
#endif
