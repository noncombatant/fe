// Copyright 2024 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#ifndef AUTO_H
#define AUTO_H

#include <stdio.h>

#include "fe.h"

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if __has_attribute(cleanup)
#define AUTO(type, name, value, destructor) \
  type name __attribute__((cleanup(destructor))) = (value)
#else
#error we depend on __attribute__((cleanup))
#endif

void CloseFile(FILE** f);
void FreeChar(char** p);
void CloseContext(FeContext** c);

#endif
