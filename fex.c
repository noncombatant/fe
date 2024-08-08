// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include "fex.h"

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn) {
  FeSet(ctx, FeMakeSymbol(ctx, name), FeMakeNativeFn(ctx, fn));
}
