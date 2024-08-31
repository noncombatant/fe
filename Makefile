CC = clang
CFLAGS = -Weverything -Werror -std=c2x -lm \
	-Wno-poison-system-directories \
	-Wno-declaration-after-statement \
	-Wno-padded \
	-Wno-switch-default \
	-Wno-pre-c23-compat \
	-Wno-unsafe-buffer-usage \
	-Wno-implicit-fallthrough \
	-Wno-unused-command-line-argument \
	-Wno-unknown-warning-option

ifdef RELEASE
	CFLAGS += -O3 -fsanitize=undefined -fsanitize-trap=all
else
	CFLAGS += -g -O3 -fsanitize=address -fsanitize=undefined -fsanitize-trap=all
endif

test:
	./test.sh

run: fe
	./fe

bench: clean
	./bench.sh

fe: main.c fe.o fex.o fex_io.o fex_math.o fex_process.o fex_re.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -rf fe *.o *.dSYM
	-rm -f scripts/*.csv scripts/*.times
