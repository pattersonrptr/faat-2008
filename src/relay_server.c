/* ccrelay — multi-client TCP relay / broker server
 *
 * Usage:
 *   ccrelay <port> [--dns <host>:<port>] [--ip <myip>]
 *
 * Accepts up to MAX_CLIENTS simultaneous clients (one thread each).
 * Every message from a client is broadcast to all other connected clients.
 * If --dns is supplied the server self-registers on startup so that
 * ccclient instances can discover it automatically.
 * --ip is the IP address to advertise in that REGISTER call
 *       (required when using --dns; defaults to 127.0.0.1).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chat.h"
#include "protocol.h"
#include "net_util.h"

/* ── Portable mutex ─────────────────────────────────────────────────────── */

#ifdef _WIN32
#  include <windows.h>
   typedef CRITICAL_SECTION mutex_t;
#  define mutex_init(m)    InitializeCriticalSection(m)
#  define mutex_lock(m)    EnterCriticalSection(m)
#  define mutex_unlock(m)  LeaveCriticalSection(m)
#  define mutex_destroy(m) DeleteCriticalSection(m)
#else
#  include <pthread.h>
   typedef pthread_mutex_t mutex_t;
#  define mutex_init(m)    pthread_mutex_init(m, NULL)
#  define mutex_lock(m)    pthread_mutex_lock(m)
#  define mutex_unlock(m)  pthread_mutex_unlock(m)
#  define mutex_destroy(m) pthread_mutex_destroy(m)
#endif

/* ── Client table ───────────────────────────────────────────────────────── */

typedef struct {
    socket_t sock;
    char     username[MAX_NAME_LEN];
    int      active;
} ClientSlot;

static ClientSlot g_clients[MAX_CLIENTS];
static mutex_t    g_mutex;

static void clients_init(void)
{
    memset(g_clients, 0, sizeof(g_clients));
    mutex_init(&g_mutex);
}

/* Returns slot index on success, -1 if full.  Caller must hold g_mutex. */
static int alloc_slot(socket_t sock)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!g_clients[i].active) {
            g_clients[i].sock         = sock;
            g_clients[i].active       = 1;
            g_clients[i].username[0]  = '\0';
            return i;
        }
    }
    return -1;
}

static void free_slot(int idx)
{
    mutex_lock(&g_mutex);
    g_clients[idx].active = 0;
    mutex_unlock(&g_mutex);
}

/* Broadcast msg to every active client except skip_idx. */
static void broadcast(const char *msg, int len, int skip_idx)
{
    mutex_lock(&g_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i].active && i != skip_idx)
            net_send_all(g_clients[i].sock, msg, len);
    }
    mutex_unlock(&g_mutex);
}

/* ── Per-client thread ──────────────────────────────────────────────────── */

typedef struct { int idx; } ClientArg;

#ifdef _WIN32
static DWORD WINAPI client_thread(LPVOID raw)
#else
static void *client_thread(void *raw)
#endif
{
    ClientArg *ca  = (ClientArg *)raw;
    int        idx = ca->idx;
    free(ca);

    socket_t sock = g_clients[idx].sock;
    char buf[MAX_MSG_LEN];
    char out[MAX_MSG_LEN + MAX_NAME_LEN + 4];

    /* ── Handshake: expect "HELLO <username>\n" ── */
    int n = net_recv_line(sock, buf, (int)sizeof(buf));
    if (n <= 0 || strncmp(buf, "HELLO ", 6) != 0) {
        sock_close(sock);
        free_slot(idx);
        goto done;
    }
    net_strip_crlf(buf);
    strncpy(g_clients[idx].username, buf + 6, MAX_NAME_LEN - 1);
    printf("[ccrelay] + %s  (slot %d)\n", g_clients[idx].username, idx);

    /* Notify all others that someone joined. */
    int len = snprintf(out, sizeof(out), "*** %s joined\n",
                       g_clients[idx].username);
    broadcast(out, len, idx);

    /* ── Forward messages ── */
    for (;;) {
        n = net_recv_line(sock, buf, (int)sizeof(buf));
        if (n <= 0) break;
        net_strip_crlf(buf);
        if (buf[0] == '\0') continue;
        len = snprintf(out, sizeof(out), "%s: %s\n",
                       g_clients[idx].username, buf);
        broadcast(out, len, idx);
    }

    /* ── Disconnected ── */
    printf("[ccrelay] - %s  (slot %d)\n", g_clients[idx].username, idx);
    len = snprintf(out, sizeof(out), "*** %s left\n",
                   g_clients[idx].username);
    broadcast(out, len, idx);

    sock_close(sock);
    free_slot(idx);

done:
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* ── DNS self-registration ──────────────────────────────────────────────── */

static void dns_register(const char *dns_host, int dns_port,
                         const char *my_ip, int my_port)
{
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", dns_port);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(dns_host, port_str, &hints, &res) != 0) {
        fprintf(stderr, "[!] dns_register: cannot resolve %s\n", dns_host);
        return;
    }

    socket_t sock = socket(res->ai_family, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCK) { freeaddrinfo(res); return; }

    /* Set a 2-second receive timeout so we don't block forever. */
#ifdef _WIN32
    DWORD tvms = 2000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tvms, sizeof(tvms));
#else
    struct timeval tv = {2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    char msg[DNS_BUF_SIZE];
    snprintf(msg, sizeof(msg), "REGISTER %s %s %d\n",
             SERVICE_NAME, my_ip, my_port);
    sendto(sock, msg, (int)strlen(msg), 0,
           res->ai_addr, (socklen_t)res->ai_addrlen);
    freeaddrinfo(res);

    char reply[DNS_BUF_SIZE];
    int n = (int)recv(sock, reply, (int)sizeof(reply) - 1, 0);
    sock_close(sock);

    if (n > 0) {
        reply[n] = '\0';
        net_strip_crlf(reply);
        printf("[ccrelay] DNS register: %s\n", reply);
    } else {
        fprintf(stderr, "[!] DNS server did not respond within 2s\n");
    }
}

/* ── Entry point ────────────────────────────────────────────────────────── */

static void print_usage(const char *prog)
{
    fprintf(stderr,
        "ccrelay — multi-client relay server\n\n"
        "Usage: %s <port> [--dns <host>:<port>] [--ip <myip>]\n\n"
        "  --dns  register with this DNS server (host:port)\n"
        "  --ip   public IP to advertise to DNS (default: 127.0.0.1)\n\n"
        "Example (same machine):\n"
        "  %s 5000 --dns localhost:5300 --ip 127.0.0.1\n",
        prog, prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[-] WSAStartup failed\n");
        return 1;
    }
#endif

    if (argc < 2) print_usage(argv[0]);

    int  relay_port = atoi(argv[1]);
    char dns_host[128] = {0};
    int  dns_port      = 0;
    char my_ip[64]     = "127.0.0.1";

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dns") == 0 && i + 1 < argc) {
            char *colon = strrchr(argv[++i], ':');
            if (colon) {
                int hlen = (int)(colon - argv[i]);
                strncpy(dns_host, argv[i], hlen < 127 ? hlen : 127);
                dns_host[hlen < 127 ? hlen : 127] = '\0';
                dns_port = atoi(colon + 1);
            } else {
                strncpy(dns_host, argv[i], sizeof(dns_host) - 1);
                dns_port = DNS_DEFAULT_PORT;
            }
        } else if (strcmp(argv[i], "--ip") == 0 && i + 1 < argc) {
            strncpy(my_ip, argv[++i], sizeof(my_ip) - 1);
        }
    }

    clients_init();

    /* ── TCP listening socket ── */
    socket_t lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock == INVALID_SOCK) {
        fprintf(stderr, "[-] socket() failed: %d\n", sock_errno());
        return 1;
    }

    int opt = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)relay_port);

    if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[-] bind() on port %d failed: %d\n",
                relay_port, sock_errno());
        sock_close(lsock);
        return 1;
    }
    listen(lsock, 8);

    /* ── Optional DNS self-registration ── */
    if (dns_host[0])
        dns_register(dns_host, dns_port, my_ip, relay_port);

    printf("[ccrelay] Relay server on TCP port %d  (max %d clients)\n",
           relay_port, MAX_CLIENTS);

    /* ── Accept loop ── */
    for (;;) {
        struct sockaddr_in peer;
        socklen_t peerlen = sizeof(peer);
        socket_t csock = accept(lsock, (struct sockaddr *)&peer, &peerlen);
        if (csock == INVALID_SOCK) continue;

        mutex_lock(&g_mutex);
        int idx = alloc_slot(csock);
        mutex_unlock(&g_mutex);

        if (idx < 0) {
            const char *full = "ERR server full\n";
            send(csock, full, (int)strlen(full), 0);
            sock_close(csock);
            continue;
        }

        ClientArg *ca = malloc(sizeof(ClientArg));
        if (!ca) { sock_close(csock); free_slot(idx); continue; }
        ca->idx = idx;

#ifdef _WIN32
        HANDLE h = CreateThread(NULL, 0, client_thread, ca, 0, NULL);
        if (h) CloseHandle(h);
        else  { free(ca); sock_close(csock); free_slot(idx); }
#else
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, ca) == 0)
            pthread_detach(tid);
        else { free(ca); sock_close(csock); free_slot(idx); }
#endif
    }

    sock_close(lsock);
    mutex_destroy(&g_mutex);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
