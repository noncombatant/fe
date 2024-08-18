CC = clang
CFLAGS = -Weverything -Werror -std=c2x -lm \
  -Wno-poison-system-directories \
  -Wno-declaration-after-statement \
  -Wno-padded \
	-Wno-switch-default \
	-Wno-pre-c23-compat \
	-Wno-unsafe-buffer-usage \
	-Wno-implicit-fallthrough \
	-Wno-unused-command-line-argument

ifdef RELEASE
	CFLAGS += -O3 -fsanitize=undefined -fsanitize-trap=all
else
	CFLAGS += -g -O3 -fsanitize=address -fsanitize=undefined -fsanitize-trap=all
endif

test: fe
	./test.sh

run: fe
	./fe

fe: main.c fe.o fex.o fex_io.o fex_math.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -rf fe *.o *.dSYM
