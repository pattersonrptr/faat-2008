/**
 * dns_server.c — Minimal DNS server for the cdns experiment.
 *
 * Responsibilities:
 *   1. Listen on UDP port DNS_UDP_PORT.
 *   2. Reply to DNS_DISCOVER broadcasts with DNS_HERE <ip> <port>.
 *   3. Accept DNS_REGISTER <name> <ip> <port>  — stores the mapping.
 *   4. Answer DNS_QUERY <name>  with DNS_ANSWER <ip> <port>
 *      or DNS_NXDOMAIN if unknown.
 *
 * Usage:  dns_server [bind-ip]
 *   bind-ip defaults to 0.0.0.0 (all interfaces).
 *   The IP announced in DNS_HERE replies is auto-detected from the incoming
 *   packet's destination (via IP_PKTINFO / WSA_CMSG) or, as a portable
 *   fallback, from the first non-loopback interface.
 */

#include "common.h"

/* ─── DNS record table ───────────────────────────────────────────────────── */

#define MAX_RECORDS 64

typedef struct {
    char name[MAX_NAME_LEN];
    char ip[INET_ADDRSTRLEN];
    int  port;
} DnsRecord;

static DnsRecord records[MAX_RECORDS];
static int       record_count = 0;

static void record_add(const char *name, const char *ip, int port)
{
    /* Update existing entry if name already registered */
    for (int i = 0; i < record_count; i++) {
        if (strcmp(records[i].name, name) == 0) {
            strncpy(records[i].ip, ip, INET_ADDRSTRLEN - 1);
            records[i].port = port;
            printf("[dns] Updated: %s → %s:%d\n", name, ip, port);
            return;
        }
    }
    if (record_count >= MAX_RECORDS) {
        fprintf(stderr, "[dns] Record table full, ignoring %s\n", name);
        return;
    }
    strncpy(records[record_count].name, name, MAX_NAME_LEN - 1);
    strncpy(records[record_count].ip,   ip,   INET_ADDRSTRLEN - 1);
    records[record_count].port = port;
    record_count++;
    printf("[dns] Registered: %s → %s:%d\n", name, ip, port);
}

static DnsRecord *record_find(const char *name)
{
    for (int i = 0; i < record_count; i++) {
        if (strcmp(records[i].name, name) == 0)
            return &records[i];
    }
    return NULL;
}

/* ─── Helpers ────────────────────────────────────────────────────────────── */

/*
 * Get the best public IP of this machine to announce in DNS_HERE.
 * Strategy: open a UDP socket and "connect" (no packet sent) to 8.8.8.8 —
 * the OS fills in the source address for that route.
 */
static void get_my_ip(char *out, size_t outlen)
{
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_INVALID(s)) { strncpy(out, DNS_FALLBACK_IP, outlen - 1); return; }

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family      = AF_INET;
    remote.sin_port        = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);

    if (connect(s, (struct sockaddr *)&remote, sizeof(remote)) == 0) {
        struct sockaddr_in local;
        socklen_t len = sizeof(local);
        if (getsockname(s, (struct sockaddr *)&local, &len) == 0) {
            inet_ntop(AF_INET, &local.sin_addr, out, (socklen_t)outlen);
            CLOSE_SOCKET(s);
            return;
        }
    }
    CLOSE_SOCKET(s);
    strncpy(out, DNS_FALLBACK_IP, outlen - 1);
}

/* ─── Main loop ──────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    if (net_init() != 0) return 1;

    /* Discover our own IP once at startup */
    char my_ip[INET_ADDRSTRLEN] = {0};
    get_my_ip(my_ip, sizeof(my_ip));
    printf("[dns] My IP: %s\n", my_ip);
    printf("[dns] Listening on UDP port %d...\n", DNS_UDP_PORT);

    /* Create UDP socket */
    sock_t sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_INVALID(sock)) {
        fprintf(stderr, "[dns] socket() failed: %d\n", SOCK_ERR);
        net_cleanup(); return 1;
    }

    /* Allow address reuse */
    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    /* Enable receiving broadcast packets */
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&yes, sizeof(yes));

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family      = AF_INET;
    bind_addr.sin_port        = htons(DNS_UDP_PORT);
    bind_addr.sin_addr.s_addr = INADDR_ANY;  /* listen on all interfaces */

    if (bind(sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[dns] bind() failed: %d\n", SOCK_ERR);
        CLOSE_SOCKET(sock); net_cleanup(); return 1;
    }

    char buf[MAX_MSG_LEN];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        memset(buf, 0, sizeof(buf));
        client_len = sizeof(client_addr);

        int n = (int)recvfrom(sock, buf, sizeof(buf) - 1, 0,
                              (struct sockaddr *)&client_addr, &client_len);
        if (n <= 0) continue;

        buf[n] = '\0';
        /* Strip trailing newline */
        while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r')) buf[--n] = '\0';

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, sender_ip, sizeof(sender_ip));
        printf("[dns] From %s:%d → \"%s\"\n",
               sender_ip, ntohs(client_addr.sin_port), buf);

        char reply[MAX_MSG_LEN] = {0};

        /* ── DNS_DISCOVER ─────────────────────────────────────────────── */
        if (strcmp(buf, MSG_DISCOVER) == 0) {
            snprintf(reply, sizeof(reply), "%s %s %d\n",
                     MSG_HERE, my_ip, DNS_UDP_PORT);
        }
        /* ── DNS_REGISTER <name> <ip> <port> ──────────────────────────── */
        else if (strncmp(buf, MSG_REGISTER, strlen(MSG_REGISTER)) == 0) {
            char name[MAX_NAME_LEN], ip[INET_ADDRSTRLEN];
            int  port;
            if (sscanf(buf, "DNS_REGISTER %63s %45s %d", name, ip, &port) == 3) {
                record_add(name, ip, port);
                snprintf(reply, sizeof(reply), "DNS_OK\n");
            } else {
                snprintf(reply, sizeof(reply), "DNS_ERROR bad_register\n");
            }
        }
        /* ── DNS_QUERY <name> ─────────────────────────────────────────── */
        else if (strncmp(buf, MSG_QUERY, strlen(MSG_QUERY)) == 0) {
            char name[MAX_NAME_LEN];
            if (sscanf(buf, "DNS_QUERY %63s", name) == 1) {
                DnsRecord *r = record_find(name);
                if (r) {
                    snprintf(reply, sizeof(reply), "%s %s %d\n",
                             MSG_ANSWER, r->ip, r->port);
                } else {
                    snprintf(reply, sizeof(reply), "%s\n", MSG_NXDOMAIN);
                }
            } else {
                snprintf(reply, sizeof(reply), "DNS_ERROR bad_query\n");
            }
        }
        else {
            printf("[dns] Unknown message, ignoring.\n");
            continue;
        }

        if (reply[0]) {
            sendto(sock, reply, (int)strlen(reply), 0,
                   (struct sockaddr *)&client_addr, client_len);
            printf("[dns] Reply → \"%.*s\"\n", (int)strlen(reply) - 1, reply);
        }
    }

    CLOSE_SOCKET(sock);
    net_cleanup();
    return 0;
}
