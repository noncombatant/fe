// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include "fex.h"

const char* FexVersion = "0.1";

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn) {
  type_names[FexTError] = "error";
  type_names[FexTFile] = "file";
  type_names[FexTRE] = "regular-expression";

  FeSet(ctx, FeMakeSymbol(ctx, name), FeMakeNativeFn(ctx, fn));
}
