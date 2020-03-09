#include <assert.h>
#include <stdlib.h>
#include <math.h>
#define _XOPEN_SOURCE /* See feature_test_macros(7) */

#include <time.h>
#include "bstring.h"
#include "bobject.h"
#include "berror.h"
#include "bparser.h"
#include "bdate.h"

#define DEFAULT_FORMATTED_STRING_LENGTH 255

static TValue * primitive_Date_now(bean_State * B, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  TValue * ret = malloc(sizeof(TValue));
  time_t timer = time(NULL);
  setnvalue(ret, timer);
  return ret;
}

static TValue * primitive_Date_build(bean_State * B, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  TValue * ret = malloc(sizeof(TValue));
  Date * date = malloc(sizeof(Date));
  time_t t = time(NULL);
  date->time = t;
  setdatevalue(ret, date);
  return ret;
}

static TValue * primitive_Date_parse(bean_State * B, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  TValue * ret = malloc(sizeof(TValue));
  Date * date = malloc(sizeof(Date));
  char * formatString = "%Y-%m-%d %H:%M:%S %z"; // default formater
  assert(argc >= 1);
  TValue * tvTime = tvalue_inspect(B, &args[0]);
  char * timeStr = getstr(svalue(tvTime));

  if (argc >= 2) {
    TValue * tvFormatter = tvalue_inspect(B, &args[1]);
    formatString = getstr(svalue(tvFormatter));
  }
  struct tm t;
  memset(&t, 0, sizeof(struct tm));
  strptime(timeStr, formatString, &t);
  date->time = mktime(&t);
  setdatevalue(ret, date);
  return ret;
}

static TValue * primitive_Date_format(bean_State * B, TValue * thisVal UNUSED, TValue * args UNUSED, int argc UNUSED) {
  assert(ttisdate(thisVal));
  TValue * ret = malloc(sizeof(TValue));
  Date * date = datevalue(thisVal);
  char * formatString = "%Y-%m-%d %H:%M:%S %z"; // default formater
  char * timezone = NULL;
  assert(argc >= 0);

  if (argc >= 1) {
    TValue * tvFormatter = tvalue_inspect(B, &args[0]);
    formatString = getstr(svalue(tvFormatter));
  }

  if (argc >= 2) {
    TValue * tvZone = tvalue_inspect(B, &args[1]);
    char * string = getstr(svalue(tvZone));
    timezone = malloc(strlen(string) + 2);
    sprintf(timezone, ":%s", string);
  }

  char * tz = NULL;

  if (timezone) {
    tz = getenv("TZ");
    setenv("TZ", timezone, 1);
  }

  struct tm * tm = localtime(&date->time);

  char s[DEFAULT_FORMATTED_STRING_LENGTH];
  strftime(s, DEFAULT_FORMATTED_STRING_LENGTH, formatString, tm);

  if (timezone) {
    tz ? setenv("TZ", tz, 1) : unsetenv("TZ");
  }

  TString * ts = beanS_newlstr(B, s, strlen(s));
  setsvalue(ret, ts);
  return ret;
}

#define FROM_BROKEN_DOWN_TIME(attribute) do { \
    assert(ttisdate(thisVal));                \
    TValue * ret = malloc(sizeof(TValue));    \
    Date * date = datevalue(thisVal);         \
    time_t time = date -> time;               \
    struct tm * t = localtime(&time);         \
    setnvalue(ret, attribute);                \
    return ret;                               \
  } while(0)                                  \

static TValue * primitive_Date_getYear(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  FROM_BROKEN_DOWN_TIME(t->tm_year + 1900);
}

static TValue * primitive_Date_getMonth(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  FROM_BROKEN_DOWN_TIME(t->tm_mon);
}

static TValue * primitive_Date_getDate(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  FROM_BROKEN_DOWN_TIME(t->tm_mday);
}

static TValue * primitive_Date_getHours(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  FROM_BROKEN_DOWN_TIME(t->tm_hour);
}

static TValue * primitive_Date_getMinutes(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  FROM_BROKEN_DOWN_TIME(t->tm_min);
}

static TValue * primitive_Date_getSeconds(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  FROM_BROKEN_DOWN_TIME(t->tm_sec);
}

static TValue * primitive_Date_getWeekDay(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  FROM_BROKEN_DOWN_TIME(t->tm_wday);
}

static TValue * primitive_Date_getTimezoneOffset(bean_State * B, TValue * thisVal, TValue * args UNUSED, int argc UNUSED) {
  // https://www.gnu.org/software/libc/manual/html_node/Time-Zone-Functions.html
  assert(ttisdate(thisVal));
  TValue * ret = malloc(sizeof(TValue));
  Date * date = datevalue(thisVal);
  time_t time = date -> time;
  struct tm * tm = localtime(&time); // Call the tzset to set the timezone from environment variable
  setnvalue(ret, tm->tm_gmtoff);
  return ret;
}

TValue * init_Date(bean_State * B) {
  srand(time(0));
  global_State * G = B->l_G;
  TValue * proto = malloc(sizeof(TValue));
  Hash * h = init_hash(B);
  sethashvalue(proto, h);
  set_prototype_function(B, "now", 3, primitive_Date_now, hhvalue(proto));
  set_prototype_function(B, "build", 5, primitive_Date_build, hhvalue(proto));
  set_prototype_function(B, "parse", 5, primitive_Date_parse, hhvalue(proto));
  /* set_prototype_function(B, "utc", 3, primitive_Date_utc, hhvalue(proto)); */
  set_prototype_function(B, "getYear", 7, primitive_Date_getYear, hhvalue(proto));
  set_prototype_function(B, "getMonth", 8, primitive_Date_getMonth, hhvalue(proto));
  set_prototype_function(B, "getDate", 7, primitive_Date_getDate, hhvalue(proto));
  set_prototype_function(B, "getHours", 8, primitive_Date_getHours, hhvalue(proto));
  set_prototype_function(B, "getMinutes", 10, primitive_Date_getMinutes, hhvalue(proto));
  set_prototype_function(B, "getSeconds", 10, primitive_Date_getSeconds, hhvalue(proto));
  set_prototype_function(B, "getWeekDay", 10, primitive_Date_getWeekDay, hhvalue(proto));
  set_prototype_function(B, "getTimezoneOffset", 17, primitive_Date_getTimezoneOffset, hhvalue(proto));
  set_prototype_function(B, "format", 6, primitive_Date_format, hhvalue(proto));

  TValue * date = malloc(sizeof(TValue));
  setsvalue(date, beanS_newlstr(B, "Date", 4));
  hash_set(B, G->globalScope->variables, date, proto);
  return proto;
}
