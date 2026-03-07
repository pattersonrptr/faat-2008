/**
 * common.h — Shared definitions, platform portability, and protocol messages.
 *
 * Supports Windows (Winsock2) and POSIX (BSD sockets) transparently.
 */

#ifndef COMMON_H
#define COMMON_H

/* ─── Platform detection ─────────────────────────────────────────────────── */

#ifdef _WIN32
#  define PLATFORM_WINDOWS
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
   typedef int socklen_t;
#  define CLOSE_SOCKET(s)  closesocket(s)
#  define SOCK_ERR         WSAGetLastError()
#  define INVALID_SOCK     INVALID_SOCKET
#  define IS_INVALID(s)    ((s) == INVALID_SOCKET)
   /* Winsock uses unsigned SOCKET, POSIX uses int */
   typedef SOCKET sock_t;
#else
#  define PLATFORM_POSIX
#  include <sys/types.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#  include <errno.h>
#  define CLOSE_SOCKET(s)  close(s)
#  define SOCK_ERR         errno
#  define INVALID_SOCK     (-1)
#  define IS_INVALID(s)    ((s) < 0)
#  define SOCKET_ERROR     (-1)
   typedef int sock_t;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ─── Ports & addresses ──────────────────────────────────────────────────── */

#define DNS_UDP_PORT       9053   /* DNS server listens for UDP messages      */
#define WEB_DEFAULT_PORT   8080   /* Web server default TCP port              */

/*
 * Fallback IP used when no broadcast reply is received in time.
 * Change this to the actual LAN IP of the machine running dns_server
 * if you want to test across multiple machines.
 */
#define DNS_FALLBACK_IP    "127.0.0.1"

/* Broadcast address — works on most /24 LANs.  For localhost-only tests on
 * Windows use the real LAN broadcast (e.g. 192.168.1.255) because Windows
 * does NOT route UDP broadcast on the loopback interface. */
#define BROADCAST_ADDR     "255.255.255.255"

/* How many milliseconds to wait for a broadcast reply before falling back */
#define DISCOVER_TIMEOUT_MS  2000

/* ─── Protocol message format (plain-text, newline-terminated) ───────────── */
/*
 *  CLIENT / WEB-SERVER  →  (UDP broadcast)  →  ALL
 *      "DNS_DISCOVER\n"
 *
 *  DNS-SERVER           →  (UDP unicast)    →  REQUESTER
 *      "DNS_HERE <ip> <port>\n"
 *
 *  WEB-SERVER           →  (UDP unicast)    →  DNS
 *      "DNS_REGISTER <name> <ip> <port>\n"
 *
 *  CLIENT               →  (UDP unicast)    →  DNS
 *      "DNS_QUERY <name>\n"
 *
 *  DNS-SERVER           →  (UDP unicast)    →  CLIENT
 *      "DNS_ANSWER <ip> <port>\n"
 *      "DNS_NXDOMAIN\n"          (name not found)
 *
 *  CLIENT               →  (TCP)            →  WEB-SERVER
 *      "GET / HTTP/1.0\r\nHost: <name>\r\n\r\n"
 *
 *  WEB-SERVER           →  (TCP)            →  CLIENT
 *      "HTTP/1.0 200 OK\r\n...\r\n\r\n<html>…</html>"
 */

#define MSG_DISCOVER    "DNS_DISCOVER"
#define MSG_HERE        "DNS_HERE"
#define MSG_REGISTER    "DNS_REGISTER"
#define MSG_QUERY       "DNS_QUERY"
#define MSG_ANSWER      "DNS_ANSWER"
#define MSG_NXDOMAIN    "DNS_NXDOMAIN"

#define MAX_NAME_LEN    64
#define MAX_MSG_LEN     256
#define MAX_HTML_LEN    8192

/* ─── Winsock init/cleanup helpers ──────────────────────────────────────── */

static inline int net_init(void)
{
#ifdef PLATFORM_WINDOWS
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[net] WSAStartup failed: %d\n", WSAGetLastError());
        return -1;
    }
#endif
    return 0;
}

static inline void net_cleanup(void)
{
#ifdef PLATFORM_WINDOWS
    WSACleanup();
#endif
}

/* ─── Portable "set socket timeout" ─────────────────────────────────────── */

static inline int sock_set_timeout(sock_t s, int ms)
{
#ifdef PLATFORM_WINDOWS
    DWORD tv = (DWORD)ms;
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec  = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

#endif /* COMMON_H */
