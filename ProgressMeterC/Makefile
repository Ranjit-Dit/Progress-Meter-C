CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean
all: progress_meter

progress_meter: progress_meter.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f progress_meter
