// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FEX_REGEX_H
#define FEX_REGEX_H

#include "fe.h"

void FexInstallRE(FeContext* ctx);

FeObject* FexCompileRE(FeContext* ctx, FeObject* arg);
FeObject* FexMatchRE(FeContext* ctx, FeObject* arg);

#endif
