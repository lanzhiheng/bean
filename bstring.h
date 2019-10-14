#ifndef BEAN_STRING_H
#define BEAN_STRING_H

#define MEMERRMSG       "not enough memory"

// Size of String is sum of the size of struct TString and the extra char array with the '\0' at the end of the string.
#define sizelstring(l)  (sizeof(TString) + ((l) + 1) * sizeof(char))

#endif
