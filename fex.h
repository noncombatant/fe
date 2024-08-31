// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FEX_H
#define FEX_H

#include "fe.h"

extern const char* FexVersion;

enum {
  FexTError = FeTFex0,
  FexTFile = FeTFex1,
  FexTRE = FeTFex2,
};

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn);

#endif
