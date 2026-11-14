SEAL_DIR = deps/seal
SEAL     = $(SEAL_DIR)/seal

CC     ?= gcc
CFLAGS  = -O2 -Wall -Wextra -fPIC -I$(SEAL_DIR)/src

all: $(SEAL) termio.so

$(SEAL):
	$(MAKE) -C $(SEAL_DIR)

SRCS = $(wildcard src/*.c)

termio.so: $(SRCS) $(wildcard src/*.h)
	$(CC) $(CFLAGS) -shared $(SRCS) -o $@

EX ?= 00
run: all
	$(SEAL) $(wildcard examples/$(EX)_*.seal)

test: all
	$(SEAL) examples/00_hello.seal
	$(SEAL) tests/keys.seal < tests/keys.in | diff -u tests/keys.expected -
	$(SEAL) tests/utf8.seal | diff -u tests/utf8.expected -
	@echo ok

clean:
	rm -f termio.so

distclean: clean
	$(MAKE) -C $(SEAL_DIR) clean

.PHONY: all run test clean distclean
