CC = clang
CFLAGS = -Weverything -std=c2x \
  -Wno-poison-system-directories \
  -Wno-declaration-after-statement \
  -Wno-padded

# Sometimes -O3 catches more warnings:
CFLAGS += -O3 -fsanitize=undefined

# Debug:
# CFLAGS += -O0 -fsanitize=address -g
# LDFLAGS = -static-libsan

run: fe
	./fe

fe: main.c fe.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -rf fe *.o *.dSYM
