// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <regex.h>

#include "fex.h"

const char* FexVersion = "0.1";

static FeObject* FexGC(FeContext* ctx, FeObject* o) {
  const FeType type = FeGetType(o);
  // TODO: Consider calling into functions exported by fex_foo.h, so we don't
  // have to #include aspects of foo implementation in this file.
  if (type == FexTRE) {
    regex_t* re = FeToPtr(ctx, o);
    regfree(re);
    free(re);
  }
  return &nil;
}

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn) {
  // TODO: This stuff should not be here, but in a new `FexInit`.
  type_names[FexTError] = "error";
  type_names[FexTFile] = "file";
  type_names[FexTRE] = "regular-expression";
  FeGetHandlers(ctx)->gc = FexGC;

  FeSet(ctx, FeMakeSymbol(ctx, name), FeMakeNativeFn(ctx, fn));
}
