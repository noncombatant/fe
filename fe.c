// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2022 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <string.h>

#include "fe.h"

#define car(x) ((x)->car.o)
#define cdr(x) ((x)->cdr.o)
#define tag(x) ((x)->car.c)
#define isnil(x) ((x) == &nil)
#define type(x) (tag(x) & 0x1 ? tag(x) >> 2 : FE_TPAIR)
// TODO: When we use a real enum for type, update this.
#define settype(x, t) (tag(x) = (char)((t) << 2 | 1))
#define number(x) ((x)->cdr.n)
#define prim(x) ((x)->cdr.c)
#define cfunc(x) ((x)->cdr.f)
#define strbuf(x) (&(x)->car.c + 1)

#define STRBUFSIZE ((int)sizeof(FeObject*) - 1)
#define GCMARKBIT (0x2)
#define GCSTACKSIZE (256)

enum {
  P_LET,
  P_SET,
  P_IF,
  P_FN,
  P_MAC,
  P_WHILE,
  P_QUOTE,
  P_AND,
  P_OR,
  P_DO,
  P_CONS,
  P_CAR,
  P_CDR,
  P_SETCAR,
  P_SETCDR,
  P_LIST,
  P_NOT,
  P_IS,
  P_ATOM,
  P_PRINT,
  P_LT,
  P_LTE,
  P_ADD,
  P_SUB,
  P_MUL,
  P_DIV,
  P_MAX
};

static const char* primnames[] = {
    "let",  "=",     "if",  "fn",  "mac",    "while",  "quote", "and", "or",
    "do",   "cons",  "car", "cdr", "setcar", "setcdr", "list",  "not", "is",
    "atom", "print", "<",   "<=",  "+",      "-",      "*",     "/"};

static const char* typenames[] = {"pair",   "free",   "nil",  "number",
                                  "symbol", "string", "func", "macro",
                                  "prim",   "cfunc",  "ptr"};

typedef union {
  FeObject* o;
  FeCFunc* f;
  FeNumber n;
  char c;
} Value;

struct FeObject {
  Value car, cdr;
};

struct FeContext {
  FeHandlers handlers;
  FeObject* gcstack[GCSTACKSIZE];
  size_t gcstack_idx;
  FeObject* objects;
  size_t object_count;
  FeObject* calllist;
  FeObject* freelist;
  FeObject* symlist;
  FeObject* t;
  char nextchr;
};

static FeObject nil = {{(void*)(FE_TNIL << 2 | 1)}, {NULL}};

FeHandlers* FeGetHandlers(FeContext* ctx) {
  return &ctx->handlers;
}

void noreturn FeHandleError(FeContext* ctx, const char* msg) {
  FeObject* cl = ctx->calllist;
  /* reset context state */
  ctx->calllist = &nil;
  /* do error handler */
  if (ctx->handlers.error) {
    ctx->handlers.error(ctx, msg, cl);
  }
  /* error handler returned -- print error and traceback, exit */
  fprintf(stderr, "error: %s\n", msg);
  for (; !isnil(cl); cl = cdr(cl)) {
    char buf[64];
    FeToString(ctx, car(cl), buf, sizeof(buf));
    fprintf(stderr, "=> %s\n", buf);
  }
  exit(EXIT_FAILURE);
}

FeObject* FeGetNextArgument(FeContext* ctx, FeObject** arg) {
  FeObject* a = *arg;
  if (type(a) != FE_TPAIR) {
    if (isnil(a)) {
      FeHandleError(ctx, "too few arguments");
    }
    FeHandleError(ctx, "dotted pair in argument list");
  }
  *arg = cdr(a);
  return car(a);
}

static FeObject* checktype(FeContext* ctx, FeObject* obj, int type) {
  char buf[64];
  if (type(obj) != type) {
    sprintf(buf, "expected %s, got %s", typenames[type], typenames[type(obj)]);
    FeHandleError(ctx, buf);
  }
  return obj;
}

int FeGetType(FeContext*, FeObject* obj) {
  return type(obj);
}

int FeIsNil(FeContext*, FeObject* obj) {
  return isnil(obj);
}

void FePushGC(FeContext* ctx, FeObject* obj) {
  if (ctx->gcstack_idx == GCSTACKSIZE) {
    FeHandleError(ctx, "gc stack overflow");
  }
  ctx->gcstack[ctx->gcstack_idx++] = obj;
}

void FeRestoreGC(FeContext* ctx, size_t idx) {
  ctx->gcstack_idx = idx;
}

size_t FeSaveGC(FeContext* ctx) {
  return ctx->gcstack_idx;
}

void FeMark(FeContext* ctx, FeObject* obj) {
  FeObject* car;
begin:
  if (tag(obj) & GCMARKBIT) {
    return;
  }
  car = car(obj); /* store car before modifying it with GCMARKBIT */
  tag(obj) |= GCMARKBIT;

  switch (type(obj)) {
    case FE_TPAIR:
      FeMark(ctx, car);
      /* fall through */
    case FE_TFUNC:
    case FE_TMACRO:
    case FE_TSYMBOL:
    case FE_TSTRING:
      obj = cdr(obj);
      goto begin;

    case FE_TPTR:
      if (ctx->handlers.mark) {
        ctx->handlers.mark(ctx, obj);
      }
      break;
  }
}

static void collectgarbage(FeContext* ctx) {
  size_t i;
  /* mark */
  for (i = 0; i < ctx->gcstack_idx; i++) {
    FeMark(ctx, ctx->gcstack[i]);
  }
  FeMark(ctx, ctx->symlist);
  /* sweep and unmark */
  for (i = 0; i < ctx->object_count; i++) {
    FeObject* obj = &ctx->objects[i];
    if (type(obj) == FE_TFREE) {
      continue;
    }
    if (~tag(obj) & GCMARKBIT) {
      if (type(obj) == FE_TPTR && ctx->handlers.gc) {
        ctx->handlers.gc(ctx, obj);
      }
      settype(obj, FE_TFREE);
      cdr(obj) = ctx->freelist;
      ctx->freelist = obj;
    } else {
      tag(obj) &= ~GCMARKBIT;
    }
  }
}

// Translated from [the original
// Java](https://floating-point-gui.de/errors/comparison/).
//
// See also
// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/.
static bool double_nearly_equal(double a, double b, double epsilon) {
  const double absA = fabs(a);
  const double absB = fabs(b);
  const double diff = fabs(a - b);
// It's OK to turn this warning off for this limited section of code, because
// it's in the context of handling tiny errors.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
  if (a == b) {
    // Special case; handles infinities.
    return true;
  } else if (a == 0 || b == 0 || (absA + absB < DBL_MIN)) {
#pragma clang diagnostic pop
    // Either a or b is zero, or both are extremely close to it. Relative error
    // is less meaningful here.
    return diff < (epsilon * DBL_MIN);
  }
  // Use relative error.
  return diff / fmin((absA + absB), DBL_MAX) < epsilon;
}

static int equal(FeObject* a, FeObject* b) {  // TODO return bool
  if (a == b) {
    return 1;
  }
  if (type(a) != type(b)) {
    return 0;
  }
  if (type(a) == FE_TNUMBER) {
    return double_nearly_equal(number(a), number(b), DBL_EPSILON);
  }
  if (type(a) == FE_TSTRING) {
    for (; !isnil(a); a = cdr(a), b = cdr(b)) {
      if (car(a) != car(b)) {
        return 0;
      }
    }
    return a == b;
  }
  return 0;
}

static int streq(FeObject* obj, const char* str) {
  while (!isnil(obj)) {
    int i;
    for (i = 0; i < STRBUFSIZE; i++) {
      if (strbuf(obj)[i] != *str) {
        return 0;
      }
      if (*str) {
        str++;
      }
    }
    obj = cdr(obj);
  }
  return *str == '\0';
}

static FeObject* object(FeContext* ctx) {
  FeObject* obj;
  /* do gc if freelist has no more objects */
  if (isnil(ctx->freelist)) {
    collectgarbage(ctx);
    if (isnil(ctx->freelist)) {
      FeHandleError(ctx, "out of memory");
    }
  }
  /* get object from freelist and push to the gcstack */
  obj = ctx->freelist;
  ctx->freelist = cdr(obj);
  FePushGC(ctx, obj);
  return obj;
}

FeObject* FeCons(FeContext* ctx, FeObject* car, FeObject* cdr) {
  FeObject* obj = object(ctx);
  car(obj) = car;
  cdr(obj) = cdr;
  return obj;
}

FeObject* FeMakeBool(FeContext* ctx, int b) {
  return b ? ctx->t : &nil;
}

FeObject* FeMakeNumber(FeContext* ctx, FeNumber n) {
  FeObject* obj = object(ctx);
  settype(obj, FE_TNUMBER);
  number(obj) = n;
  return obj;
}

static FeObject* buildstring(FeContext* ctx, FeObject* tail, char chr) {
  if (!tail || strbuf(tail)[STRBUFSIZE - 1] != '\0') {
    FeObject* obj = FeCons(ctx, NULL, &nil);
    settype(obj, FE_TSTRING);
    if (tail) {
      cdr(tail) = obj;
      ctx->gcstack_idx--;
    }
    tail = obj;
  }
  strbuf(tail)[strlen(strbuf(tail))] = chr;
  return tail;
}

FeObject* FeMakeString(FeContext* ctx, const char* str) {
  FeObject* obj = buildstring(ctx, NULL, '\0');
  FeObject* tail = obj;
  while (*str) {
    tail = buildstring(ctx, tail, *str++);
  }
  return obj;
}

FeObject* FeMakeSymbol(FeContext* ctx, const char* name) {
  FeObject* obj;
  /* try to find in symlist */
  for (obj = ctx->symlist; !isnil(obj); obj = cdr(obj)) {
    if (streq(car(cdr(car(obj))), name)) {
      return car(obj);
    }
  }
  /* create new object, push to symlist and return */
  obj = object(ctx);
  settype(obj, FE_TSYMBOL);
  cdr(obj) = FeCons(ctx, FeMakeString(ctx, name), &nil);
  ctx->symlist = FeCons(ctx, obj, ctx->symlist);
  return obj;
}

FeObject* FeMakeCFunc(FeContext* ctx, FeCFunc fn) {
  FeObject* obj = object(ctx);
  settype(obj, FE_TCFUNC);
  cfunc(obj) = fn;
  return obj;
}

FeObject* FeMakePtr(FeContext* ctx, void* ptr) {
  FeObject* obj = object(ctx);
  settype(obj, FE_TPTR);
  cdr(obj) = ptr;
  return obj;
}

FeObject* FeMakeList(FeContext* ctx, FeObject** objs, int n) {
  FeObject* res = &nil;
  while (n--) {
    res = FeCons(ctx, objs[n], res);
  }
  return res;
}

FeObject* FeCar(FeContext* ctx, FeObject* obj) {
  if (isnil(obj)) {
    return obj;
  }
  return car(checktype(ctx, obj, FE_TPAIR));
}

FeObject* FeCdr(FeContext* ctx, FeObject* obj) {
  if (isnil(obj)) {
    return obj;
  }
  return cdr(checktype(ctx, obj, FE_TPAIR));
}

static void writestr(FeContext* ctx, FeWriteFn fn, void* udata, const char* s) {
  while (*s) {
    fn(ctx, udata, *s++);
  }
}

void FeWrite(FeContext* ctx, FeObject* obj, FeWriteFn fn, void* udata, int qt) {
  char buf[32];

  switch (type(obj)) {
    case FE_TNIL:
      writestr(ctx, fn, udata, "nil");
      break;

    case FE_TNUMBER:
      sprintf(buf, "%.7g", number(obj));
      writestr(ctx, fn, udata, buf);
      break;

    case FE_TPAIR:
      fn(ctx, udata, '(');
      for (;;) {
        FeWrite(ctx, car(obj), fn, udata, 1);
        obj = cdr(obj);
        if (type(obj) != FE_TPAIR) {
          break;
        }
        fn(ctx, udata, ' ');
      }
      if (!isnil(obj)) {
        writestr(ctx, fn, udata, " . ");
        FeWrite(ctx, obj, fn, udata, 1);
      }
      fn(ctx, udata, ')');
      break;

    case FE_TSYMBOL:
      FeWrite(ctx, car(cdr(obj)), fn, udata, 0);
      break;

    case FE_TSTRING:
      if (qt) {
        fn(ctx, udata, '"');
      }
      while (!isnil(obj)) {
        int i;
        for (i = 0; i < STRBUFSIZE && strbuf(obj)[i]; i++) {
          if (qt && strbuf(obj)[i] == '"') {
            fn(ctx, udata, '\\');
          }
          fn(ctx, udata, strbuf(obj)[i]);
        }
        obj = cdr(obj);
      }
      if (qt) {
        fn(ctx, udata, '"');
      }
      break;

    default:
      sprintf(buf, "[%s %p]", typenames[type(obj)], (void*)obj);
      writestr(ctx, fn, udata, buf);
      break;
  }
}

static void writefp(FeContext*, void* udata, char chr) {
  fputc(chr, udata);
}

void FeWriteFile(FeContext* ctx, FeObject* obj, FILE* fp) {
  FeWrite(ctx, obj, writefp, fp, 0);
}

typedef struct {
  char* p;
  int n;
} CharPtrInt;

static void writebuf(FeContext*, void* udata, char chr) {
  CharPtrInt* x = udata;
  if (x->n) {
    *x->p++ = chr;
    x->n--;
  }
}

int FeToString(FeContext* ctx, FeObject* obj, char* dst, int size) {
  CharPtrInt x;
  x.p = dst;
  x.n = size - 1;
  FeWrite(ctx, obj, writebuf, &x, 0);
  *x.p = '\0';
  return size - x.n - 1;
}

FeNumber FeToNumber(FeContext* ctx, FeObject* obj) {
  return number(checktype(ctx, obj, FE_TNUMBER));
}

void* FeToPtr(FeContext* ctx, FeObject* obj) {
  return cdr(checktype(ctx, obj, FE_TPTR));
}

static FeObject* getbound(FeObject* sym, FeObject* env) {
  /* try to find in environment */
  for (; !isnil(env); env = cdr(env)) {
    FeObject* x = car(env);
    if (car(x) == sym) {
      return x;
    }
  }
  /* return global */
  return cdr(sym);
}

void FeSet(FeContext*, FeObject* sym, FeObject* v) {
  cdr(getbound(sym, &nil)) = v;
}

static FeObject rparen;

static FeObject* read_(FeContext* ctx, FeReadFn fn, void* udata) {
  const char* delimiter = " \n\t\r();";
  FeObject *v, *res, **tail;
  FeNumber n;
  char chr;
  size_t gc;
  char buf[64], *p;

  /* get next character */
  chr = ctx->nextchr ? ctx->nextchr : fn(ctx, udata);
  ctx->nextchr = '\0';

  /* skip whitespace */
  while (chr && strchr(" \n\t\r", chr)) {
    chr = fn(ctx, udata);
  }

  switch (chr) {
    case '\0':
      return NULL;

    case ';':
      while (chr && chr != '\n') {
        chr = fn(ctx, udata);
      }
      return read_(ctx, fn, udata);

    case ')':
      return &rparen;

    case '(':
      res = &nil;
      tail = &res;
      gc = FeSaveGC(ctx);
      FePushGC(ctx, res); /* to cause error on too-deep nesting */
      while ((v = read_(ctx, fn, udata)) != &rparen) {
        if (v == NULL) {
          FeHandleError(ctx, "unclosed list");
        }
        if (type(v) == FE_TSYMBOL && streq(car(cdr(v)), ".")) {
          /* dotted pair */
          *tail = FeRead(ctx, fn, udata);
        } else {
          /* proper pair */
          *tail = FeCons(ctx, v, &nil);
          tail = &cdr(*tail);
        }
        FeRestoreGC(ctx, gc);
        FePushGC(ctx, res);
      }
      return res;

    case '\'':
      v = FeRead(ctx, fn, udata);
      if (!v) {
        FeHandleError(ctx, "stray '''");
      }
      return FeCons(ctx, FeMakeSymbol(ctx, "quote"), FeCons(ctx, v, &nil));

    case '"':
      res = buildstring(ctx, NULL, '\0');
      v = res;
      chr = fn(ctx, udata);
      while (chr != '"') {
        if (chr == '\0') {
          FeHandleError(ctx, "unclosed string");
        }
        if (chr == '\\') {
          chr = fn(ctx, udata);
          if (strchr("nrt", chr)) {
            chr = strchr("n\nr\rt\t", chr)[1];
          }
        }
        v = buildstring(ctx, v, chr);
        chr = fn(ctx, udata);
      }
      return res;

    default:
      p = buf;
      do {
        if (p == buf + sizeof(buf) - 1) {
          FeHandleError(ctx, "symbol too long");
        }
        *p++ = chr;
        chr = fn(ctx, udata);
      } while (chr && !strchr(delimiter, chr));
      *p = '\0';
      ctx->nextchr = chr;
      n = strtod(buf, &p); /* try to read as number */
      if (p != buf && strchr(delimiter, *p)) {
        return FeMakeNumber(ctx, n);
      }
      if (!strcmp(buf, "nil")) {
        return &nil;
      }
      return FeMakeSymbol(ctx, buf);
  }
}

FeObject* FeRead(FeContext* ctx, FeReadFn fn, void* udata) {
  FeObject* obj = read_(ctx, fn, udata);
  if (obj == &rparen) {
    FeHandleError(ctx, "stray ')'");
  }
  return obj;
}

static char readfp(FeContext*, void* udata) {
  int c = fgetc(udata);
  return c == EOF ? '\0' : (char)c;
}

FeObject* FeReadFile(FeContext* ctx, FILE* fp) {
  return FeRead(ctx, readfp, fp);
}

static FeObject* eval(FeContext* ctx,
                      FeObject* obj,
                      FeObject* env,
                      FeObject** bind);

static FeObject* evallist(FeContext* ctx, FeObject* lst, FeObject* env) {
  FeObject* res = &nil;
  FeObject** tail = &res;
  while (!isnil(lst)) {
    *tail =
        FeCons(ctx, eval(ctx, FeGetNextArgument(ctx, &lst), env, NULL), &nil);
    tail = &cdr(*tail);
  }
  return res;
}

static FeObject* dolist(FeContext* ctx, FeObject* lst, FeObject* env) {
  FeObject* res = &nil;
  size_t save = FeSaveGC(ctx);
  while (!isnil(lst)) {
    FeRestoreGC(ctx, save);
    FePushGC(ctx, lst);
    FePushGC(ctx, env);
    res = eval(ctx, FeGetNextArgument(ctx, &lst), env, &env);
  }
  return res;
}

static FeObject* argstoenv(FeContext* ctx,
                           FeObject* prm,
                           FeObject* arg,
                           FeObject* env) {
  while (!isnil(prm)) {
    if (type(prm) != FE_TPAIR) {
      env = FeCons(ctx, FeCons(ctx, prm, arg), env);
      break;
    }
    env = FeCons(ctx, FeCons(ctx, car(prm), FeCar(ctx, arg)), env);
    prm = cdr(prm);
    arg = FeCdr(ctx, arg);
  }
  return env;
}

#define evalarg() eval(ctx, FeGetNextArgument(ctx, &arg), env, NULL)

#define arithop(op)                          \
  {                                          \
    FeNumber x = FeToNumber(ctx, evalarg()); \
    while (!isnil(arg)) {                    \
      x = x op FeToNumber(ctx, evalarg());   \
    }                                        \
    res = FeMakeNumber(ctx, x);              \
  }

#define numcmpop(op)                                 \
  {                                                  \
    va = checktype(ctx, evalarg(), FE_TNUMBER);      \
    vb = checktype(ctx, evalarg(), FE_TNUMBER);      \
    res = FeMakeBool(ctx, number(va) op number(vb)); \
  }

static FeObject* eval(FeContext* ctx,
                      FeObject* obj,
                      FeObject* env,
                      FeObject** newenv) {
  FeObject *fn, *arg, *res;
  FeObject cl, *va, *vb;
  size_t n, gc;

  if (type(obj) == FE_TSYMBOL) {
    return cdr(getbound(obj, env));
  }
  if (type(obj) != FE_TPAIR) {
    return obj;
  }

  car(&cl) = obj;
  cdr(&cl) = ctx->calllist;
  ctx->calllist = &cl;

  gc = FeSaveGC(ctx);
  fn = eval(ctx, car(obj), env, NULL);
  arg = cdr(obj);
  res = &nil;

  switch (type(fn)) {
    case FE_TPRIM:
      switch (prim(fn)) {
        case P_LET:
          va = checktype(ctx, FeGetNextArgument(ctx, &arg), FE_TSYMBOL);
          if (newenv) {
            *newenv = FeCons(ctx, FeCons(ctx, va, evalarg()), env);
          }
          break;

        case P_SET:
          va = checktype(ctx, FeGetNextArgument(ctx, &arg), FE_TSYMBOL);
          cdr(getbound(va, env)) = evalarg();
          break;

        case P_IF:
          while (!isnil(arg)) {
            va = evalarg();
            if (!isnil(va)) {
              res = isnil(arg) ? va : evalarg();
              break;
            }
            if (isnil(arg)) {
              break;
            }
            arg = cdr(arg);
          }
          break;

        case P_FN:
        case P_MAC:
          va = FeCons(ctx, env, arg);
          FeGetNextArgument(ctx, &arg);
          res = object(ctx);
          settype(res, prim(fn) == P_FN ? FE_TFUNC : FE_TMACRO);
          cdr(res) = va;
          break;

        case P_WHILE:
          va = FeGetNextArgument(ctx, &arg);
          n = FeSaveGC(ctx);
          while (!isnil(eval(ctx, va, env, NULL))) {
            dolist(ctx, arg, env);
            FeRestoreGC(ctx, n);
          }
          break;

        case P_QUOTE:
          res = FeGetNextArgument(ctx, &arg);
          break;

        case P_AND:
          while (!isnil(arg) && !isnil(res = evalarg()))
            ;
          break;

        case P_OR:
          while (!isnil(arg) && isnil(res = evalarg()))
            ;
          break;

        case P_DO:
          res = dolist(ctx, arg, env);
          break;

        case P_CONS:
          va = evalarg();
          res = FeCons(ctx, va, evalarg());
          break;

        case P_CAR:
          res = FeCar(ctx, evalarg());
          break;

        case P_CDR:
          res = FeCdr(ctx, evalarg());
          break;

        case P_SETCAR:
          va = checktype(ctx, evalarg(), FE_TPAIR);
          car(va) = evalarg();
          break;

        case P_SETCDR:
          va = checktype(ctx, evalarg(), FE_TPAIR);
          cdr(va) = evalarg();
          break;

        case P_LIST:
          res = evallist(ctx, arg, env);
          break;

        case P_NOT:
          res = FeMakeBool(ctx, isnil(evalarg()));
          break;

        case P_IS:
          va = evalarg();
          res = FeMakeBool(ctx, equal(va, evalarg()));
          break;

        case P_ATOM:
          res = FeMakeBool(ctx, FeGetType(ctx, evalarg()) != FE_TPAIR);
          break;

        case P_PRINT:
          while (!isnil(arg)) {
            FeWriteFile(ctx, evalarg(), stdout);
            if (!isnil(arg)) {
              printf(" ");
            }
          }
          printf("\n");
          break;

        case P_LT:
          numcmpop(<);
          break;
        case P_LTE:
          numcmpop(<=);
          break;
        case P_ADD:
          arithop(+);
          break;
        case P_SUB:
          arithop(-);
          break;
        case P_MUL:
          arithop(*);
          break;
        case P_DIV:
          arithop(/);
          break;
      }
      break;

    case FE_TCFUNC:
      res = cfunc(fn)(ctx, evallist(ctx, arg, env));
      break;

    case FE_TFUNC:
      arg = evallist(ctx, arg, env);
      va = cdr(fn); /* (env params ...) */
      vb = cdr(va); /* (params ...) */
      res = dolist(ctx, cdr(vb), argstoenv(ctx, car(vb), arg, car(va)));
      break;

    case FE_TMACRO:
      va = cdr(fn); /* (env params ...) */
      vb = cdr(va); /* (params ...) */
      /* replace caller object with code generated by macro and re-eval */
      *obj = *dolist(ctx, cdr(vb), argstoenv(ctx, car(vb), arg, car(va)));
      FeRestoreGC(ctx, gc);
      ctx->calllist = cdr(&cl);
      return eval(ctx, obj, env, NULL);

    default:
      FeHandleError(ctx, "tried to call non-callable value");
  }

  FeRestoreGC(ctx, gc);
  FePushGC(ctx, res);
  ctx->calllist = cdr(&cl);
  return res;
}

FeObject* FeEvaluate(FeContext* ctx, FeObject* obj) {
  return eval(ctx, obj, &nil, NULL);
}

FeContext* FeOpenContext(void* arena, size_t size) {
  // Initialize the context:
  FeContext* ctx = arena;
  memset(ctx, 0, sizeof(FeContext));
  arena = (char*)arena + sizeof(FeContext);
  size -= sizeof(FeContext);

  // Initialize the objects memory region:
  ctx->objects = (FeObject*)arena;
  ctx->object_count = size / sizeof(FeObject);

  // Initialize the lists:
  ctx->calllist = &nil;
  ctx->freelist = &nil;
  ctx->symlist = &nil;

  // Populate the freelist:
  for (size_t i = 0; i < ctx->object_count; i++) {
    FeObject* obj = &ctx->objects[i];
    settype(obj, FE_TFREE);
    cdr(obj) = ctx->freelist;
    ctx->freelist = obj;
  }

  // Initialize the objects:
  ctx->t = FeMakeSymbol(ctx, "t");
  FeSet(ctx, ctx->t, ctx->t);

  // Register the built-in primitives:
  size_t save = FeSaveGC(ctx);
  for (size_t i = 0; i < P_MAX; i++) {
    FeObject* v = object(ctx);
    settype(v, FE_TPRIM);
    prim(v) = (char)i;
    FeSet(ctx, FeMakeSymbol(ctx, primnames[i]), v);
    FeRestoreGC(ctx, save);
  }
  return ctx;
}

void FeCloseContext(FeContext* ctx) {
  // Clear the GC stack and symbol list: this makes all objects unreachable:
  ctx->gcstack_idx = 0;
  ctx->symlist = &nil;
  collectgarbage(ctx);
}
