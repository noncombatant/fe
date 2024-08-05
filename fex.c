// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <math.h>

#include "fex.h"

void FexInstallNativeFn(FeContext* ctx, const char* name, FeNativeFn fn) {
  FeSet(ctx, FeMakeSymbol(ctx, name), FeMakeNativeFn(ctx, fn));
}

void FexInstallNativeFns(FeContext* ctx) {
  FexInstallNativeFn(ctx, "abs", FexAbs);
  FexInstallNativeFn(ctx, "ceiling", FexCeiling);
  FexInstallNativeFn(ctx, "cube-root", FexCubeRoot);
  FexInstallNativeFn(ctx, "floor", FexFloor);
  FexInstallNativeFn(ctx, "hypotenuse", FexHypotenuse);
  FexInstallNativeFn(ctx, "is-finite", FexIsFinite);
  FexInstallNativeFn(ctx, "is-infinite", FexIsInfinite);
  FexInstallNativeFn(ctx, "is-nan", FexIsNaN);
  FexInstallNativeFn(ctx, "is-normal", FexIsNormal);
  FexInstallNativeFn(ctx, "lg", FexLg);
  FexInstallNativeFn(ctx, "log", FexLog);
  FexInstallNativeFn(ctx, "max", FexMax);
  FexInstallNativeFn(ctx, "min", FexMin);
  FexInstallNativeFn(ctx, "%", FexModulus);
  FexInstallNativeFn(ctx, "nearby-int", FexNearbyInt);
  FexInstallNativeFn(ctx, "pow", FexPow);
  FexInstallNativeFn(ctx, "remainder", FexRemainder);
  FexInstallNativeFn(ctx, "round", FexRound);
  FexInstallNativeFn(ctx, "round-to-int", FexRoundToInt);
  FexInstallNativeFn(ctx, "square-root", FexSquareRoot);
  FexInstallNativeFn(ctx, "trunc", FexTruncate);
}

FeObject* FexAbs(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, fabs(x));
}

FeObject* FexCeiling(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, ceil(x));
}

FeObject* FexCubeRoot(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, cbrt(x));
}

FeObject* FexFloor(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, floor(x));
}

FeObject* FexHypotenuse(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, hypot(x, y));
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

FeObject* FexLg(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, log2(x));
}

FeObject* FexLog(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, log(x));
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

FeObject* FexModulus(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, fmod(x, y));
}

// FeObject* FexNaN(FeContext* ctx, FeObject* arg) {
//   char tag[64];
//   (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), tag, sizeof(tag));
//   return FeMakeNumber(ctx, nan(tag));
// }

FeObject* FexNearbyInt(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, nearbyint(x));
}

FeObject* FexPow(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, pow(x, y));
}

FeObject* FexRemainder(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, remainder(x, y));
}

FeObject* FexRound(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, round(x));
}

FeObject* FexRoundToInt(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, rint(x));
}

FeObject* FexSquareRoot(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, sqrt(x));
}

FeObject* FexTruncate(FeContext* ctx, FeObject* arg) {
  double x = FeToNumber(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeNumber(ctx, trunc(x));
}
