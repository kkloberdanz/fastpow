CC=cc
CFLAGS=$(OPT) -Wall -Wextra -Wpedantic -std=gnu89
LDFLAGS=$(OPT) -lm

.PHONY: release
release: OPT=-O3 -march=native -mtune=native -ggdb3
release: all

.PHONY: debug
debug: OPT=-O0 -ggdb3
debug: all

.PHONY: all
all: fastpow pow-lookup-table.bin gentable

fastpow: fastpow.c
	$(CC) -o fastpow fastpow.c $(CFLAGS) $(LDFLAGS)

pow-lookup-table.bin: gentable
	./gentable

gentable: gentable.c
	$(CC) -o gentable gentable.c $(CFLAGS) $(LDFLAGS)

.PHONY: perf
perf: fastpow pow-lookup-table.bin
	perf record ./fastpow
	perf report

.PHONY: clean
clean:
	rm -f pow-lookup-table.bin
	rm -f gentable
	rm -f fastpow
	rm -f perf.data*
