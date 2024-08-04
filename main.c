// Copyright 2020 rxi, https://github.com/rxi/fe
// Copyright 2022 Chris Palmer, https://noncombatant.org/
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <unistd.h>

#include "fe.h"

static jmp_buf top_level;

static void noreturn HandleError(FeContext*, const char* message, FeObject*) {
  fprintf(stderr, "error: %s\n", message);
  longjmp(top_level, -1);
}

static void noreturn PrintHelp(int status) {
  // TODO
  exit(status);
}

int main(int count, char* arguments[]) {
  // Parse command line options:
  size_t arena_size = 64 * 1024;
  while (true) {
    int ch = getopt(count, arguments, "hs:");
    if (ch == -1) {
      break;
    }
    switch (ch) {
      case 'h':
        PrintHelp(EXIT_SUCCESS);
      case 's': {
        char* end = NULL;
        arena_size = strtoul(optarg, &end, 0);
        if (end == optarg) {
          PrintHelp(EXIT_FAILURE);
        }
        break;
      }
      default:
        PrintHelp(EXIT_FAILURE);
    }
  }
  count -= optind;
  arguments += optind;

  // Initialize the context:
  char* arena = malloc(arena_size);
  FeContext* ctx = fe_open(arena, arena_size);

  FILE* input = stdin;
  if (count > 0) {
    input = fopen(arguments[0], "rb");
    if (!input) {
      fe_error(ctx, "could not open input file");
    }
  }

  if (input == stdin) {
    fe_handlers(ctx)->error = HandleError;
  }
  size_t gc = fe_savegc(ctx);
  setjmp(top_level);

  // REPL:
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
  free(arena);
}
