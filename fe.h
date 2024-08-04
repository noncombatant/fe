// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2022 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FE_H
#define FE_H

#include <stdio.h>
#include <stdlib.h>

#define FE_VERSION "1.0"

typedef double FeNumber;
typedef struct FeObject FeObject;
typedef struct FeContext FeContext;
typedef FeObject* FeCFunc(FeContext* ctx, FeObject* args);
typedef void FeErrorFn(FeContext* ctx, const char* err, FeObject* cl);
typedef void FeWriteFn(FeContext* ctx, void* udata, char chr);
typedef char FeReadFn(FeContext* ctx, void* udata);

typedef struct {
  FeErrorFn* error;
  FeCFunc* mark;
  FeCFunc* gc;
} FeHandlers;

enum {
  FE_TPAIR,
  FE_TFREE,
  FE_TNIL,
  FE_TNUMBER,
  FE_TSYMBOL,
  FE_TSTRING,
  FE_TFUNC,
  FE_TMACRO,
  FE_TPRIM,
  FE_TCFUNC,
  FE_TPTR
};

FeContext* fe_open(void* ptr, size_t size);
void fe_close(FeContext* ctx);
FeHandlers* fe_handlers(FeContext* ctx);
void fe_error(FeContext* ctx, const char* msg);
FeObject* fe_nextarg(FeContext* ctx, FeObject** arg);
int fe_type(FeContext* ctx, FeObject* obj);   // TODO enum return
int fe_isnil(FeContext* ctx, FeObject* obj);  // TODO bool
void fe_pushgc(FeContext* ctx, FeObject* obj);
void fe_restoregc(FeContext* ctx, size_t idx);
size_t fe_savegc(FeContext* ctx);
void fe_mark(FeContext* ctx, FeObject* obj);
FeObject* fe_cons(FeContext* ctx, FeObject* car, FeObject* cdr);
FeObject* fe_bool(FeContext* ctx, int b);  // TODO use `bool`
FeObject* fe_number(FeContext* ctx, FeNumber n);
FeObject* fe_string(FeContext* ctx, const char* str);
FeObject* fe_symbol(FeContext* ctx, const char* name);
FeObject* fe_cfunc(FeContext* ctx, FeCFunc fn);
FeObject* fe_ptr(FeContext* ctx, void* ptr);
FeObject* fe_list(FeContext* ctx, FeObject** objs, int n);
FeObject* fe_car(FeContext* ctx, FeObject* obj);
FeObject* fe_cdr(FeContext* ctx, FeObject* obj);
void fe_write(FeContext* ctx, FeObject* obj, FeWriteFn fn, void* udata, int qt);
void fe_writefp(FeContext* ctx, FeObject* obj, FILE* fp);
int fe_tostring(FeContext* ctx, FeObject* obj, char* dst, int size);
FeNumber fe_tonumber(FeContext* ctx, FeObject* obj);
void* fe_toptr(FeContext* ctx, FeObject* obj);
void fe_set(FeContext* ctx, FeObject* sym, FeObject* v);
FeObject* fe_read(FeContext* ctx, FeReadFn fn, void* udata);
FeObject* fe_readfp(FeContext* ctx, FILE* fp);
FeObject* fe_eval(FeContext* ctx, FeObject* obj);

#endif
