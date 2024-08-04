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
#define settype(x, t) (tag(x) = (t) << 2 | 1)
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

FeHandlers* fe_handlers(FeContext* ctx) {
  return &ctx->handlers;
}

void noreturn fe_error(FeContext* ctx, const char* msg) {
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
    fe_tostring(ctx, car(cl), buf, sizeof(buf));
    fprintf(stderr, "=> %s\n", buf);
  }
  exit(EXIT_FAILURE);
}

FeObject* fe_nextarg(FeContext* ctx, FeObject** arg) {
  FeObject* a = *arg;
  if (type(a) != FE_TPAIR) {
    if (isnil(a)) {
      fe_error(ctx, "too few arguments");
    }
    fe_error(ctx, "dotted pair in argument list");
  }
  *arg = cdr(a);
  return car(a);
}

static FeObject* checktype(FeContext* ctx, FeObject* obj, int type) {
  char buf[64];
  if (type(obj) != type) {
    sprintf(buf, "expected %s, got %s", typenames[type], typenames[type(obj)]);
    fe_error(ctx, buf);
  }
  return obj;
}

int fe_type(FeContext*, FeObject* obj) {
  return type(obj);
}

int fe_isnil(FeContext*, FeObject* obj) {
  return isnil(obj);
}

void fe_pushgc(FeContext* ctx, FeObject* obj) {
  if (ctx->gcstack_idx == GCSTACKSIZE) {
    fe_error(ctx, "gc stack overflow");
  }
  ctx->gcstack[ctx->gcstack_idx++] = obj;
}

void fe_restoregc(FeContext* ctx, size_t idx) {
  ctx->gcstack_idx = idx;
}

size_t fe_savegc(FeContext* ctx) {
  return ctx->gcstack_idx;
}

void fe_mark(FeContext* ctx, FeObject* obj) {
  FeObject* car;
begin:
  if (tag(obj) & GCMARKBIT) {
    return;
  }
  car = car(obj); /* store car before modifying it with GCMARKBIT */
  tag(obj) |= GCMARKBIT;

  switch (type(obj)) {
    case FE_TPAIR:
      fe_mark(ctx, car);
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
    fe_mark(ctx, ctx->gcstack[i]);
  }
  fe_mark(ctx, ctx->symlist);
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
      fe_error(ctx, "out of memory");
    }
  }
  /* get object from freelist and push to the gcstack */
  obj = ctx->freelist;
  ctx->freelist = cdr(obj);
  fe_pushgc(ctx, obj);
  return obj;
}

FeObject* fe_cons(FeContext* ctx, FeObject* car, FeObject* cdr) {
  FeObject* obj = object(ctx);
  car(obj) = car;
  cdr(obj) = cdr;
  return obj;
}

FeObject* fe_bool(FeContext* ctx, int b) {
  return b ? ctx->t : &nil;
}

FeObject* fe_number(FeContext* ctx, FeNumber n) {
  FeObject* obj = object(ctx);
  settype(obj, FE_TNUMBER);
  number(obj) = n;
  return obj;
}

static FeObject* buildstring(FeContext* ctx, FeObject* tail, char chr) {
  if (!tail || strbuf(tail)[STRBUFSIZE - 1] != '\0') {
    FeObject* obj = fe_cons(ctx, NULL, &nil);
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

FeObject* fe_string(FeContext* ctx, const char* str) {
  FeObject* obj = buildstring(ctx, NULL, '\0');
  FeObject* tail = obj;
  while (*str) {
    tail = buildstring(ctx, tail, *str++);
  }
  return obj;
}

FeObject* fe_symbol(FeContext* ctx, const char* name) {
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
  cdr(obj) = fe_cons(ctx, fe_string(ctx, name), &nil);
  ctx->symlist = fe_cons(ctx, obj, ctx->symlist);
  return obj;
}

FeObject* fe_cfunc(FeContext* ctx, FeCFunc fn) {
  FeObject* obj = object(ctx);
  settype(obj, FE_TCFUNC);
  cfunc(obj) = fn;
  return obj;
}

FeObject* fe_ptr(FeContext* ctx, void* ptr) {
  FeObject* obj = object(ctx);
  settype(obj, FE_TPTR);
  cdr(obj) = ptr;
  return obj;
}

FeObject* fe_list(FeContext* ctx, FeObject** objs, int n) {
  FeObject* res = &nil;
  while (n--) {
    res = fe_cons(ctx, objs[n], res);
  }
  return res;
}

FeObject* fe_car(FeContext* ctx, FeObject* obj) {
  if (isnil(obj)) {
    return obj;
  }
  return car(checktype(ctx, obj, FE_TPAIR));
}

FeObject* fe_cdr(FeContext* ctx, FeObject* obj) {
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

void fe_write(FeContext* ctx,
              FeObject* obj,
              FeWriteFn fn,
              void* udata,
              int qt) {
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
        fe_write(ctx, car(obj), fn, udata, 1);
        obj = cdr(obj);
        if (type(obj) != FE_TPAIR) {
          break;
        }
        fn(ctx, udata, ' ');
      }
      if (!isnil(obj)) {
        writestr(ctx, fn, udata, " . ");
        fe_write(ctx, obj, fn, udata, 1);
      }
      fn(ctx, udata, ')');
      break;

    case FE_TSYMBOL:
      fe_write(ctx, car(cdr(obj)), fn, udata, 0);
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

void fe_writefp(FeContext* ctx, FeObject* obj, FILE* fp) {
  fe_write(ctx, obj, writefp, fp, 0);
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

int fe_tostring(FeContext* ctx, FeObject* obj, char* dst, int size) {
  CharPtrInt x;
  x.p = dst;
  x.n = size - 1;
  fe_write(ctx, obj, writebuf, &x, 0);
  *x.p = '\0';
  return size - x.n - 1;
}

FeNumber fe_tonumber(FeContext* ctx, FeObject* obj) {
  return number(checktype(ctx, obj, FE_TNUMBER));
}

void* fe_toptr(FeContext* ctx, FeObject* obj) {
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

void fe_set(FeContext*, FeObject* sym, FeObject* v) {
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
      gc = fe_savegc(ctx);
      fe_pushgc(ctx, res); /* to cause error on too-deep nesting */
      while ((v = read_(ctx, fn, udata)) != &rparen) {
        if (v == NULL) {
          fe_error(ctx, "unclosed list");
        }
        if (type(v) == FE_TSYMBOL && streq(car(cdr(v)), ".")) {
          /* dotted pair */
          *tail = fe_read(ctx, fn, udata);
        } else {
          /* proper pair */
          *tail = fe_cons(ctx, v, &nil);
          tail = &cdr(*tail);
        }
        fe_restoregc(ctx, gc);
        fe_pushgc(ctx, res);
      }
      return res;

    case '\'':
      v = fe_read(ctx, fn, udata);
      if (!v) {
        fe_error(ctx, "stray '''");
      }
      return fe_cons(ctx, fe_symbol(ctx, "quote"), fe_cons(ctx, v, &nil));

    case '"':
      res = buildstring(ctx, NULL, '\0');
      v = res;
      chr = fn(ctx, udata);
      while (chr != '"') {
        if (chr == '\0') {
          fe_error(ctx, "unclosed string");
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
          fe_error(ctx, "symbol too long");
        }
        *p++ = chr;
        chr = fn(ctx, udata);
      } while (chr && !strchr(delimiter, chr));
      *p = '\0';
      ctx->nextchr = chr;
      n = strtod(buf, &p); /* try to read as number */
      if (p != buf && strchr(delimiter, *p)) {
        return fe_number(ctx, n);
      }
      if (!strcmp(buf, "nil")) {
        return &nil;
      }
      return fe_symbol(ctx, buf);
  }
}

FeObject* fe_read(FeContext* ctx, FeReadFn fn, void* udata) {
  FeObject* obj = read_(ctx, fn, udata);
  if (obj == &rparen) {
    fe_error(ctx, "stray ')'");
  }
  return obj;
}

static char readfp(FeContext*, void* udata) {
  int c = fgetc(udata);
  return c == EOF ? '\0' : (char)c;
}

FeObject* fe_readfp(FeContext* ctx, FILE* fp) {
  return fe_read(ctx, readfp, fp);
}

static FeObject* eval(FeContext* ctx,
                      FeObject* obj,
                      FeObject* env,
                      FeObject** bind);

static FeObject* evallist(FeContext* ctx, FeObject* lst, FeObject* env) {
  FeObject* res = &nil;
  FeObject** tail = &res;
  while (!isnil(lst)) {
    *tail = fe_cons(ctx, eval(ctx, fe_nextarg(ctx, &lst), env, NULL), &nil);
    tail = &cdr(*tail);
  }
  return res;
}

static FeObject* dolist(FeContext* ctx, FeObject* lst, FeObject* env) {
  FeObject* res = &nil;
  size_t save = fe_savegc(ctx);
  while (!isnil(lst)) {
    fe_restoregc(ctx, save);
    fe_pushgc(ctx, lst);
    fe_pushgc(ctx, env);
    res = eval(ctx, fe_nextarg(ctx, &lst), env, &env);
  }
  return res;
}

static FeObject* argstoenv(FeContext* ctx,
                           FeObject* prm,
                           FeObject* arg,
                           FeObject* env) {
  while (!isnil(prm)) {
    if (type(prm) != FE_TPAIR) {
      env = fe_cons(ctx, fe_cons(ctx, prm, arg), env);
      break;
    }
    env = fe_cons(ctx, fe_cons(ctx, car(prm), fe_car(ctx, arg)), env);
    prm = cdr(prm);
    arg = fe_cdr(ctx, arg);
  }
  return env;
}

#define evalarg() eval(ctx, fe_nextarg(ctx, &arg), env, NULL)

#define arithop(op)                           \
  {                                           \
    FeNumber x = fe_tonumber(ctx, evalarg()); \
    while (!isnil(arg)) {                     \
      x = x op fe_tonumber(ctx, evalarg());   \
    }                                         \
    res = fe_number(ctx, x);                  \
  }

#define numcmpop(op)                              \
  {                                               \
    va = checktype(ctx, evalarg(), FE_TNUMBER);   \
    vb = checktype(ctx, evalarg(), FE_TNUMBER);   \
    res = fe_bool(ctx, number(va) op number(vb)); \
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

  car(&cl) = obj, cdr(&cl) = ctx->calllist;
  ctx->calllist = &cl;

  gc = fe_savegc(ctx);
  fn = eval(ctx, car(obj), env, NULL);
  arg = cdr(obj);
  res = &nil;

  switch (type(fn)) {
    case FE_TPRIM:
      switch (prim(fn)) {
        case P_LET:
          va = checktype(ctx, fe_nextarg(ctx, &arg), FE_TSYMBOL);
          if (newenv) {
            *newenv = fe_cons(ctx, fe_cons(ctx, va, evalarg()), env);
          }
          break;

        case P_SET:
          va = checktype(ctx, fe_nextarg(ctx, &arg), FE_TSYMBOL);
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
          va = fe_cons(ctx, env, arg);
          fe_nextarg(ctx, &arg);
          res = object(ctx);
          settype(res, prim(fn) == P_FN ? FE_TFUNC : FE_TMACRO);
          cdr(res) = va;
          break;

        case P_WHILE:
          va = fe_nextarg(ctx, &arg);
          n = fe_savegc(ctx);
          while (!isnil(eval(ctx, va, env, NULL))) {
            dolist(ctx, arg, env);
            fe_restoregc(ctx, n);
          }
          break;

        case P_QUOTE:
          res = fe_nextarg(ctx, &arg);
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
          res = fe_cons(ctx, va, evalarg());
          break;

        case P_CAR:
          res = fe_car(ctx, evalarg());
          break;

        case P_CDR:
          res = fe_cdr(ctx, evalarg());
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
          res = fe_bool(ctx, isnil(evalarg()));
          break;

        case P_IS:
          va = evalarg();
          res = fe_bool(ctx, equal(va, evalarg()));
          break;

        case P_ATOM:
          res = fe_bool(ctx, fe_type(ctx, evalarg()) != FE_TPAIR);
          break;

        case P_PRINT:
          while (!isnil(arg)) {
            fe_writefp(ctx, evalarg(), stdout);
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
      fe_restoregc(ctx, gc);
      ctx->calllist = cdr(&cl);
      return eval(ctx, obj, env, NULL);

    default:
      fe_error(ctx, "tried to call non-callable value");
  }

  fe_restoregc(ctx, gc);
  fe_pushgc(ctx, res);
  ctx->calllist = cdr(&cl);
  return res;
}

FeObject* fe_eval(FeContext* ctx, FeObject* obj) {
  return eval(ctx, obj, &nil, NULL);
}

FeContext* fe_open(void* arena, size_t size) {
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
  ctx->t = fe_symbol(ctx, "t");
  fe_set(ctx, ctx->t, ctx->t);

  // Register the built-in primitives:
  size_t save = fe_savegc(ctx);
  for (size_t i = 0; i < P_MAX; i++) {
    FeObject* v = object(ctx);
    settype(v, FE_TPRIM);
    prim(v) = (char)i;
    fe_set(ctx, fe_symbol(ctx, primnames[i]), v);
    fe_restoregc(ctx, save);
  }
  return ctx;
}

void fe_close(FeContext* ctx) {
  // Clear the GC stack and symbol list: this makes all objects unreachable:
  ctx->gcstack_idx = 0;
  ctx->symlist = &nil;
  collectgarbage(ctx);
}
