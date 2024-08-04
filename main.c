// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2022 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdio.h>
#include <stdnoreturn.h>
#include <unistd.h>

#include "fe.h"

static jmp_buf toplevel;
// TODO: Make this a command line option.
static char arena[64000];

static void noreturn onerror(fe_Context*, const char* message, fe_Object*) {
  fprintf(stderr, "error: %s\n", message);
  longjmp(toplevel, -1);
}

int main(int count, char* arguments[]) {
  size_t gc;
  fe_Object* obj;
  FILE* fp = stdin;
  fe_Context* ctx = fe_open(arena, sizeof(arena));

  /* init input file */
  if (count > 1) {
    fp = fopen(arguments[1], "rb");
    if (!fp) {
      fe_error(ctx, "could not open input file");
    }
  }

  if (fp == stdin) {
    fe_handlers(ctx)->error = onerror;
  }
  gc = fe_savegc(ctx);
  setjmp(toplevel);

  /* re(p)l */
  for (;;) {
    fe_restoregc(ctx, gc);
    if (fp == stdin) {
      printf("fe > ");
    }
    if (!(obj = fe_readfp(ctx, fp))) {
      break;
    }
    obj = fe_eval(ctx, obj);
    if (fp == stdin) {
      fe_writefp(ctx, obj, stdout);
      printf("\n");
    }
  }
}
