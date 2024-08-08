CC = clang
CFLAGS = -Weverything -Werror -std=c2x \
  -Wno-poison-system-directories \
  -Wno-declaration-after-statement \
  -Wno-padded

ifdef RELEASE
	CFLAGS += -O3
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
