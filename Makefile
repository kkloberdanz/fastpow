CC=cc
CFLAGS=$(OPT) -Wall -Wextra -Wpedantic -std=gnu89
LDFLAGS=$(OPT) -lm

.PHONY: release
release: OPT=-O3 -march=native -mtune=native -ggdb3
release: all

.PHONY: debug
debug: OPT=-O0 -ggdb3 -march=native -mtune=native
debug: all

.PHONY: sanitize
sanitize: OPT=-fsanitize=address -ggdb3 -march=native -mtune=native
sanitize: all

.PHONY: all
all: fastpow pow-lookup-table.bin gentable

fastpow: fastpow.c
	$(CC) -o fastpow fastpow.c $(CFLAGS) $(LDFLAGS)

pow-lookup-table.bin: gentable
	./gentable

gentable: gentable.c
	$(CC) -o gentable gentable.c $(CFLAGS) $(LDFLAGS)

# to enable perf, as root run:
# echo "-1" | tee /proc/sys/kernel/perf_event_paranoid
.PHONY: perf
perf: release
	perf record ./fastpow
	perf report

.PHONY: clean
clean:
	rm -f pow-lookup-table.bin
	rm -f gentable
	rm -f fastpow
	rm -f perf.data*
