/* ccdns — lightweight UDP service-discovery server
 *
 * Usage:  ccdns [port]   (default: DNS_DEFAULT_PORT = 5300)
 *
 * Maintains an in-memory table of { service → ip:port } entries.
 * Handles REGISTER and QUERY commands (see protocol.h).
 *
 * Single-threaded: processes one UDP datagram at a time (requests are tiny
 * and fast, so there is no need for concurrency here).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chat.h"
#include "protocol.h"
#include "net_util.h"

#define MAX_SERVICES 16

typedef struct {
    char service[64];
    char ip[INET_ADDRSTRLEN];
    int  port;
} ServiceEntry;

static ServiceEntry g_table[MAX_SERVICES];
static int          g_count = 0;

/* ── lookup / mutation ──────────────────────────────────────────────────── */

static ServiceEntry *find_service(const char *name)
{
    for (int i = 0; i < g_count; i++)
        if (strcmp(g_table[i].service, name) == 0)
            return &g_table[i];
    return NULL;
}

/* ── request handler ────────────────────────────────────────────────────── */

static void handle_msg(const char *msg, char *reply, int replysize)
{
    char svc[64], ip[64];
    int  port = 0;

    if (sscanf(msg, "REGISTER %63s %63s %d", svc, ip, &port) == 3) {
        ServiceEntry *e = find_service(svc);
        if (!e) {
            if (g_count >= MAX_SERVICES) {
                snprintf(reply, replysize, "ERR table full\n");
                return;
            }
            e = &g_table[g_count++];
        }
        strncpy(e->service, svc, sizeof(e->service) - 1);
        strncpy(e->ip,      ip,  sizeof(e->ip)      - 1);
        e->port = port;
        printf("[ccdns] Registered  %s  →  %s:%d\n", svc, ip, port);
        snprintf(reply, replysize, "OK\n");

    } else if (sscanf(msg, "QUERY %63s", svc) == 1) {
        ServiceEntry *e = find_service(svc);
        if (e) {
            snprintf(reply, replysize, "OK %s:%d\n", e->ip, e->port);
            printf("[ccdns] Query  %s  →  %s:%d\n", svc, e->ip, e->port);
        } else {
            snprintf(reply, replysize, "ERR not found\n");
            printf("[ccdns] Query  %s  →  not found\n", svc);
        }
    } else {
        snprintf(reply, replysize, "ERR unknown command\n");
    }
}

/* ── entry point ────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[-] WSAStartup failed\n");
        return 1;
    }
#endif

    int port = DNS_DEFAULT_PORT;
    if (argc >= 2) port = atoi(argv[1]);

    socket_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCK) {
        fprintf(stderr, "[-] socket() failed: %d\n", sock_errno());
        return 1;
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[-] bind() failed on UDP port %d: %d\n", port, sock_errno());
        sock_close(sock);
        return 1;
    }

    printf("[ccdns] DNS server listening on UDP port %d\n", port);

    for (;;) {
        char buf[DNS_BUF_SIZE];
        char reply[DNS_BUF_SIZE];
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);

        int n = recvfrom(sock, buf, (int)sizeof(buf) - 1, 0,
                         (struct sockaddr *)&from, &fromlen);
        if (n <= 0) continue;
        buf[n] = '\0';
        net_strip_crlf(buf);

        handle_msg(buf, reply, (int)sizeof(reply));

        sendto(sock, reply, (int)strlen(reply), 0,
               (struct sockaddr *)&from, fromlen);
    }

    sock_close(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
