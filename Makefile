SEAL_DIR = deps/seal
SEAL     = $(SEAL_DIR)/seal

CC     ?= gcc
CFLAGS  = -O2 -Wall -Wextra -fPIC -I$(SEAL_DIR)/src

all: $(SEAL) term.so

$(SEAL):
	$(MAKE) -C $(SEAL_DIR)

SRCS = $(wildcard src/*.c)

term.so: $(SRCS) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -shared $(SRCS) -o $@

EX ?= 00
run: all
	$(SEAL) $(wildcard examples/$(EX)_*.seal)

clean:
	rm -f term.so

distclean: clean
	$(MAKE) -C $(SEAL_DIR) clean

.PHONY: all run clean distclean
