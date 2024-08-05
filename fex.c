// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <math.h>

#include "fex.h"

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn) {
  FeSet(ctx, FeMakeSymbol(ctx, name), FeMakeNativeFn(ctx, fn));
}

void FexInstallNativeFns(FeContext* ctx) {
  FexInstallNativeFn(ctx, "abs", FexAbs);
  FexInstallNativeFn(ctx, "is-finite", FexIsFinite);
  FexInstallNativeFn(ctx, "is-infinite", FexIsInfinite);
  FexInstallNativeFn(ctx, "is-nan", FexIsNaN);
  FexInstallNativeFn(ctx, "is-normal", FexIsNormal);
  FexInstallNativeFn(ctx, "max", FexMax);
  FexInstallNativeFn(ctx, "min", FexMin);
  FexInstallNativeFn(ctx, "pow", FexPow);
}

FeObject* FexAbs(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, fabs(x));
}

FeObject* FexIsFinite(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isfinite(x));
}

FeObject* FexIsInfinite(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isinf(x));
}

FeObject* FexIsNaN(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isnan(x));
}

FeObject* FexIsNormal(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isnormal(x));
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
