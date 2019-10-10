#ifndef _BEAN_VM_H
#define _BEAN_VM_H
#include "common.h"

typedef union GCObject GCObject;
#define CommonHeader GCObject * next; bool mark
#define ttype(o) (o.type)
#define iscollectable(o) (ttype(o) >= BEAN_TSTRING)

struct vm {

};

typedef enum {
  BEAN_TNIL,
  BEAN_TBOOLEAN,
  BEAN_TNUMBER,
  BEAN_TSTRING,
  BEAN_TTABLE,
  BEAN_TFUNCTION
} ValueType;


typedef struct Table {
  CommonHeader;
  byte flags;

} Table;

typedef struct String {
  CommonHeader;
  uint32_t length;
  char * data;
} String;

typedef struct Number {
  double num;
} Number;


typedef struct GCHeader {
  CommonHeader;
} GCheader;

union GCObject {
  GCheader gch;
  String str;
  Table table;
};

struct value {
  ValueType type;
  union {
    GCObject * gc;
    Number n;
  };
};



#endif
