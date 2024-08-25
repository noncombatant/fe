// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef FEX_PROCESS_H
#define FEX_PROCESS_H

#include "fe.h"

void FexInstallProcess(FeContext* ctx);

FeObject* FexExecute(FeContext* ctx, FeObject* arg);

#endif
