/* ccclient — relay chat client with optional DNS discovery
 *
 * Usage:
 *   ccclient <username> --dns <host>:<port>        ← discover relay via DNS
 *   ccclient <username> --server <host>:<port>     ← connect directly
 *
 * The client first resolves the relay server address (via DNS QUERY or a
 * direct --server argument), then connects over TCP and starts chatting.
 *
 * Protocol (see protocol.h):
 *   1. Send  "HELLO <username>\n"
 *   2. Send  "<raw text>\n"  for each message
 *   3. Receive  "<username>: <text>\n"  and  "*** ...\n"  from server
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chat.h"
#include "protocol.h"
#include "net_util.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#endif

/* ── DNS query ──────────────────────────────────────────────────────────── */

/* Sends a QUERY to the DNS server over UDP and parses the "OK ip:port"
   response.  Returns 0 on success, -1 on failure. */
static int dns_query(const char *dns_host, int dns_port,
                     char *out_ip, int ip_len, int *out_port)
{
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", dns_port);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(dns_host, port_str, &hints, &res) != 0) return -1;

    socket_t sock = socket(res->ai_family, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCK) { freeaddrinfo(res); return -1; }

    /* 3-second receive timeout */
#ifdef _WIN32
    DWORD tvms = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tvms, sizeof(tvms));
#else
    struct timeval tv = {3, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    char msg[DNS_BUF_SIZE];
    snprintf(msg, sizeof(msg), "QUERY %s\n", SERVICE_NAME);
    sendto(sock, msg, (int)strlen(msg), 0,
           res->ai_addr, (socklen_t)res->ai_addrlen);
    freeaddrinfo(res);

    char reply[DNS_BUF_SIZE];
    int n = (int)recv(sock, reply, (int)sizeof(reply) - 1, 0);
    sock_close(sock);

    if (n <= 0) return -1;
    reply[n] = '\0';
    net_strip_crlf(reply);

    /* Expected format: "OK ip:port" */
    if (strncmp(reply, "OK ", 3) != 0) return -1;
    char *colon = strrchr(reply + 3, ':');
    if (!colon) return -1;

    int hlen = (int)(colon - (reply + 3));
    strncpy(out_ip, reply + 3, hlen < ip_len - 1 ? hlen : ip_len - 1);
    out_ip[hlen < ip_len - 1 ? hlen : ip_len - 1] = '\0';
    *out_port = atoi(colon + 1);
    return 0;
}

/* ── Receive thread ─────────────────────────────────────────────────────── */

typedef struct { socket_t sock; } RecvArg;

#ifdef _WIN32
static DWORD WINAPI recv_thread(LPVOID raw)
#else
static void *recv_thread(void *raw)
#endif
{
    RecvArg *ra = (RecvArg *)raw;
    char buf[MAX_MSG_LEN];

    for (;;) {
        int n = net_recv_line(ra->sock, buf, (int)sizeof(buf));
        if (n <= 0) {
            printf("\r[*] Disconnected from relay server. Press Enter to exit.\n");
            fflush(stdout);
            break;
        }
        net_strip_crlf(buf);
        printf("\r%s\n> ", buf);
        fflush(stdout);
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* ── Chat loop ──────────────────────────────────────────────────────────── */

static void run_chat(socket_t sock, const char *username)
{
    /* Handshake */
    char hello[MAX_NAME_LEN + 8];
    int hlen = snprintf(hello, sizeof(hello), "HELLO %s\n", username);
    if (net_send_all(sock, hello, hlen) <= 0) {
        fprintf(stderr, "[-] Failed to send HELLO\n");
        return;
    }

    printf("[*] Connected!  Type a message and press Enter.\n"
           "[*] /quit to disconnect.\n\n");

    /* Heap-allocate RecvArg so the pointer stays valid if run_chat returns
       before the recv thread has fully exited. */
    RecvArg *ra = malloc(sizeof(RecvArg));
    if (!ra) { fprintf(stderr, "[-] Out of memory\n"); return; }
    ra->sock = sock;

#ifdef _WIN32
    HANDLE hThread = CreateThread(NULL, 0, recv_thread, ra, 0, NULL);
    if (!hThread) {
        fprintf(stderr, "[-] CreateThread failed: %lu\n", GetLastError());
        free(ra);
        return;
    }
#else
    pthread_t tid;
    if (pthread_create(&tid, NULL, recv_thread, ra) != 0) {
        perror("[-] pthread_create");
        free(ra);
        return;
    }
    pthread_detach(tid);
#endif

    char input[MAX_MSG_LEN];
    char out[MAX_MSG_LEN + 2];

    for (;;) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, (int)sizeof(input), stdin)) break;
        net_strip_crlf(input);
        if (input[0] == '\0') continue;
        if (strcmp(input, "/quit") == 0) {
            printf("[*] Disconnecting...\n");
            break;
        }
        int len = snprintf(out, sizeof(out), "%s\n", input);
        if (net_send_all(sock, out, len) <= 0) {
            printf("[*] Send failed — connection lost.\n");
            break;
        }
    }

    shutdown(sock, SHUT_RDWR);

#ifdef _WIN32
    WaitForSingleObject(hThread, 3000);
    CloseHandle(hThread);
    free(ra);
#endif
    /* On Linux the detached thread will exit when recv returns <=0. ra is
       leaked intentionally — the process exits shortly after anyway. */
}

/* ── Usage ──────────────────────────────────────────────────────────────── */

static void usage(const char *prog)
{
    fprintf(stderr,
        "ccclient — relay chat client\n\n"
        "Usage:\n"
        "  %s <username> --dns <host>:<port>       discover relay via DNS\n"
        "  %s <username> --server <host>:<port>    connect directly\n\n"
        "Examples (same machine):\n"
        "  %s Alice --dns localhost:5300\n"
        "  %s Bob   --server localhost:5000\n\n"
        "Commands inside chat:\n"
        "  /quit   disconnect and exit\n",
        prog, prog, prog, prog);
    exit(EXIT_FAILURE);
}

/* ── Entry point ────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[-] WSAStartup failed\n");
        return 1;
    }
#endif

    if (argc < 4) usage(argv[0]);

    const char *username = argv[1];
    char server_ip[64]   = {0};
    int  server_port     = 0;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--dns") == 0 && i + 1 < argc) {
            char dns_host[128] = {0};
            int  dns_port      = DNS_DEFAULT_PORT;
            char *colon = strrchr(argv[++i], ':');
            if (colon) {
                int hlen = (int)(colon - argv[i]);
                strncpy(dns_host, argv[i], hlen < 127 ? hlen : 127);
                dns_host[hlen < 127 ? hlen : 127] = '\0';
                dns_port = atoi(colon + 1);
            } else {
                strncpy(dns_host, argv[i], sizeof(dns_host) - 1);
            }
            printf("[*] Querying DNS at %s:%d...\n", dns_host, dns_port);
            if (dns_query(dns_host, dns_port,
                          server_ip, sizeof(server_ip), &server_port) != 0) {
                fprintf(stderr,
                    "[-] DNS query failed — is ccdns running at %s:%d?\n",
                    dns_host, dns_port);
                return 1;
            }
            printf("[*] Relay server found at %s:%d\n", server_ip, server_port);

        } else if (strcmp(argv[i], "--server") == 0 && i + 1 < argc) {
            char *colon = strrchr(argv[++i], ':');
            if (!colon) {
                fprintf(stderr, "[-] --server expects host:port\n");
                return 1;
            }
            int hlen = (int)(colon - argv[i]);
            strncpy(server_ip, argv[i], hlen < 63 ? hlen : 63);
            server_ip[hlen < 63 ? hlen : 63] = '\0';
            server_port = atoi(colon + 1);
        }
    }

    if (server_ip[0] == '\0' || server_port <= 0) {
        fprintf(stderr, "[-] No relay server address — use --dns or --server\n");
        return 1;
    }

    /* ── TCP connect to relay ── */
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", server_port);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_ip, port_str, &hints, &res) != 0) {
        fprintf(stderr, "[-] Cannot resolve %s\n", server_ip);
        return 1;
    }

    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCK) {
        fprintf(stderr, "[-] socket() failed: %d\n", sock_errno());
        freeaddrinfo(res);
        return 1;
    }

    printf("[*] Connecting to relay server %s:%d...\n", server_ip, server_port);
    if (connect(sock, res->ai_addr, (socklen_t)res->ai_addrlen) < 0) {
        fprintf(stderr,
            "[-] connect() failed — is ccrelay running? (error %d)\n",
            sock_errno());
        sock_close(sock);
        freeaddrinfo(res);
        return 1;
    }
    freeaddrinfo(res);

    run_chat(sock, username);

    sock_close(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
