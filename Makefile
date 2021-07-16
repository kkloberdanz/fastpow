CC=cc
CFLAGS=$(OPT) -Wall -Wextra -Wpedantic -std=gnu89
LDFLAGS=$(OPT) -lm

.PHONY: release
release: OPT=-O3
release: all

.PHONY: debug
debug: OPT=-O0 -ggdb3
debug: all

.PHONY: all
all: fastpow pow-lookup-table.bin gentable

fastpow: fastpow.c pow-lookup-table.bin
	$(CC) -o fastpow fastpow.c $(CFLAGS) $(LDFLAGS)

pow-lookup-table.bin: gentable
	./gentable

gentable: gentable.c
	$(CC) -o gentable gentable.c $(CFLAGS) $(LDFLAGS)

clean:
	rm -f pow-lookup-table.bin
	rm -f gentable
	rm -f fastpow
