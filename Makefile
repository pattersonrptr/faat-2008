## Makefile (POSIX / MinGW-make compatible)
##
## Targets:
##   make          — build all three programs
##   make clean    — remove build artifacts

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c11 -g

## Windows (MinGW / MSYS2): link Winsock
ifeq ($(OS),Windows_NT)
  LDLIBS  = -lws2_32
  EXT     = .exe
else
  LDLIBS  =
  EXT     =
endif

ALL = dns_server$(EXT) web_server$(EXT) client$(EXT)

.PHONY: all clean

all: $(ALL)

dns_server$(EXT): dns_server.c common.h
	$(CC) $(CFLAGS) -o $@ dns_server.c $(LDLIBS)

web_server$(EXT): web_server.c common.h
	$(CC) $(CFLAGS) -o $@ web_server.c $(LDLIBS)

client$(EXT): client.c common.h
	$(CC) $(CFLAGS) -o $@ client.c $(LDLIBS)

clean:
	rm -f $(ALL)
