// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2022 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include <unistd.h>

#include "fe.h"

static jmp_buf top_level;
// TODO: Make this a command line option.
static char arena[64000];

static void noreturn onerror(FeContext*, const char* message, FeObject*) {
  fprintf(stderr, "error: %s\n", message);
  longjmp(top_level, -1);
}

int main(int count, char* arguments[]) {
  FeContext* ctx = fe_open(arena, sizeof(arena));

  /* init input file */
  FILE* input = stdin;
  if (count > 1) {
    input = fopen(arguments[1], "rb");
    if (!input) {
      fe_error(ctx, "could not open input file");
    }
  }

  if (input == stdin) {
    fe_handlers(ctx)->error = onerror;
  }
  size_t gc = fe_savegc(ctx);
  setjmp(top_level);

  /* re(p)l */
  while (true) {
    fe_restoregc(ctx, gc);
    if (input == stdin) {
      printf("fe > ");
    }
    FeObject* obj = fe_readfp(ctx, input);
    if (obj == NULL) {
      break;
    }
    obj = fe_eval(ctx, obj);
    if (input == stdin) {
      fe_writefp(ctx, obj, stdout);
      printf("\n");
    }
  }
}
