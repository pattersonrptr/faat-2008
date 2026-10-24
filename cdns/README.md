# cdns

A minimal **custom DNS + web server** system written in C, designed as a LAN service-discovery experiment.

Rebuild of a Networks course assignment at FAAT.

Three programs form the stack:

| Binary | Description |
|---|---|
| `dns_server` | UDP server that handles broadcast discovery, service registration, and name resolution |
| `web_server` | TCP HTTP server that auto-registers itself with the DNS server on startup |
| `client` | CLI client that discovers the DNS server, resolves a name, and renders the HTTP response in the terminal |

## How it works

```
client  ──UDP broadcast──▶  dns_server  (DNS_DISCOVER)
        ◀── DNS_HERE ──────

client  ──UDP──▶  dns_server  (DNS_QUERY meusite.local)
        ◀── DNS_ANSWER ip:port ──

client  ──TCP──▶  web_server  (HTTP GET)
        ◀── HTTP response ──
```

The DNS server is found automatically via UDP broadcast on the LAN.  
If no reply arrives within the timeout, the client and web server fall back to `DNS_FALLBACK_IP` (default `127.0.0.1`).

## Building

Requires **GCC** and **GNU Make**. On Windows, use [MSYS2/MinGW](https://www.msys2.org/) or a compatible toolchain.

```bash
make
```

This produces `dns_server`, `web_server`, and `client` (`.exe` on Windows).

```bash
make clean   # remove build artifacts
```

## Usage

Start each program in a separate terminal:

```bash
# 1. Start the DNS server (optionally bind to a specific IP)
./dns_server [bind-ip]

# 2. Start the web server (registers itself as "meusite.local" by default)
./web_server [site-name] [port]

# 3. Run the client
./client [site-name]
```

Default site name is `meusite.local`, default web port is `8080`, DNS UDP port is `9053`.

## Platform support

| Platform | Status |
|---|---|
| Linux / macOS | ✓ POSIX sockets |
| Windows (MinGW) | ✓ Winsock2 (`-lws2_32`) |

> **Note (Windows):** UDP broadcast does **not** route on the Windows loopback interface. For single-machine testing on Windows, set `DNS_FALLBACK_IP` in `common.h` to your real LAN IP, or use two different machines.

## Project structure

```
common.h        — shared definitions, platform portability macros, protocol constants
dns_server.c    — DNS server (UDP)
web_server.c    — HTTP web server (TCP) + DNS registration
client.c        — CLI HTTP client + DNS discovery
Makefile
build.bat       — convenience build script for Windows CMD
```

## License

[MIT](../LICENSE).
