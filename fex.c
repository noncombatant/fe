// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <math.h>

#include "fex.h"

void FexInstallNativeFn(FeContext* ctx, const char* name, FeCFunc fn) {
  FeSet(ctx, FeMakeSymbol(ctx, name), FeMakeCFunc(ctx, fn));
}

void FexInstallNativeFns(FeContext* ctx) {
  FexInstallNativeFn(ctx, "max", FexMax);
  FexInstallNativeFn(ctx, "min", FexMin);
  FexInstallNativeFn(ctx, "pow", FexPow);
}

FeObject* FexMax(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, fmax(x, y));
}

FeObject* FexMin(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, fmin(x, y));
}

FeObject* FexPow(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, pow(x, y));
}
