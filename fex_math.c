// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <math.h>

#include "fex.h"
#include "fex_math.h"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif
#ifndef M_E
#define M_E 2.718281828459045
#endif

void FexInstallMath(FeContext* ctx) {
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
  FexInstallNativeFn(ctx, "truncate", FexTruncate);

  FeSet(ctx, FeMakeSymbol(ctx, "pi"), FeMakeDouble(ctx, M_PI));
  FeSet(ctx, FeMakeSymbol(ctx, "e"), FeMakeDouble(ctx, M_E));
}

FeObject* FexAbs(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, fabs(x));
}

FeObject* FexCeiling(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, ceil(x));
}

FeObject* FexCubeRoot(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, cbrt(x));
}

FeObject* FexFloor(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, floor(x));
}

FeObject* FexHypotenuse(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, hypot(x, y));
}

FeObject* FexIsFinite(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isfinite(x));
}

FeObject* FexIsInfinite(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isinf(x));
}

FeObject* FexIsNaN(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isnan(x));
}

FeObject* FexIsNormal(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeBool(ctx, isnormal(x));
}

FeObject* FexLg(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, log2(x));
}

FeObject* FexLog(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, log(x));
}

FeObject* FexMax(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, fmax(x, y));
}

FeObject* FexMin(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, fmin(x, y));
}

FeObject* FexModulus(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, fmod(x, y));
}

// FeObject* FexNaN(FeContext* ctx, FeObject* arg) {
//   char tag[64];
//   (void)FeToString(ctx, FeGetNextArgument(ctx, &arg), tag, sizeof(tag));
//   return FeMakeDouble(ctx, nan(tag));
// }

FeObject* FexNearbyInt(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, nearbyint(x));
}

FeObject* FexPow(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, pow(x, y));
}

FeObject* FexRemainder(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  double y = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, remainder(x, y));
}

FeObject* FexRound(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, round(x));
}

FeObject* FexRoundToInt(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, rint(x));
}

FeObject* FexSquareRoot(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, sqrt(x));
}

FeObject* FexTruncate(FeContext* ctx, FeObject* arg) {
  double x = FeToDouble(ctx, FeGetNextArgument(ctx, &arg));
  return FeMakeDouble(ctx, trunc(x));
}
