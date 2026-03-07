CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude -std=c11

NET_UTIL = src/net_util.c

ifeq ($(OS),Windows_NT)
    E    = .exe
    LIBS = -lws2_32
else
    E    =
    LIBS = -lpthread
endif

# ── Targets ──────────────────────────────────────────────────────────────
# cchat    — original P2P chat  (server / client direct)
# ccdns    — UDP service-discovery server
# ccrelay  — multi-client TCP relay broker
# ccclient — relay client with DNS discovery

ALL = cchat$(E) ccdns$(E) ccrelay$(E) ccclient$(E)

.PHONY: all clean

all: $(ALL)

cchat$(E): src/main.c src/chat.c $(NET_UTIL)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

ccdns$(E): src/dns_server.c $(NET_UTIL)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

ccrelay$(E): src/relay_server.c $(NET_UTIL)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

ccclient$(E): src/relay_client.c $(NET_UTIL)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(ALL)
