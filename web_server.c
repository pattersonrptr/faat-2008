/**
 * web_server.c — Minimal HTTP web server for the cdns experiment.
 *
 * Startup sequence:
 *   1. Bind a TCP socket on WEB_DEFAULT_PORT (or port from argv).
 *   2. Discover the DNS server via UDP broadcast (DNS_DISCOVER).
 *      Falls back to DNS_FALLBACK_IP:DNS_UDP_PORT if no reply within timeout.
 *   3. Register with the DNS server: DNS_REGISTER <name> <myip> <myport>.
 *   4. Loop: accept TCP connections, serve a simple HTTP response.
 *
 * Usage:  web_server [site-name] [port]
 *   site-name defaults to "meusite.local"
 *   port      defaults to WEB_DEFAULT_PORT (8080)
 */

#include "common.h"

/* ─── The HTML page served by this web server ───────────────────────────── */

static const char *HTML_PAGE =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head><title>Experimento CDNS</title></head>\n"
    "<body>\n"
    "  <h1>Bem-vindo ao Experimento CDNS!</h1>\n"
    "  <h2>Como chegou aqui?</h2>\n"
    "  <p>Voce fez um <b>broadcast</b> na rede para encontrar o servidor DNS.</p>\n"
    "  <p>O DNS te disse onde eu estava e voce se conectou via <b>TCP</b>.</p>\n"
    "  <h2>Componentes</h2>\n"
    "  <p>Este experimento tem tres partes:</p>\n"
    "  <ul>\n"
    "    <li><b>dns_server</b> — Servidor DNS customizado (UDP)</li>\n"
    "    <li><b>web_server</b> — Este servidor HTTP (TCP)</li>\n"
    "    <li><b>client</b>    — Cliente que descobriu tudo via broadcast</li>\n"
    "  </ul>\n"
    "  <h2>Conclusao</h2>\n"
    "  <p>DNS funciona como uma <b>agenda telefonica</b> da internet.</p>\n"
    "  <p>Sem ele, voce precisaria saber o IP de cada servidor de cabeca!</p>\n"
    "</body>\n"
    "</html>\n";

/* ─── DNS discovery ──────────────────────────────────────────────────────── */

/*
 * Returns 1 on success and fills dns_ip / dns_port.
 * Tries broadcast first; falls back to DNS_FALLBACK_IP if no reply.
 */
static int discover_dns(char *dns_ip, int *dns_port)
{
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_INVALID(s)) return 0;

    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)&yes, sizeof(yes));
    sock_set_timeout(s, DISCOVER_TIMEOUT_MS);

    /* Build broadcast destination */
    struct sockaddr_in bcast;
    memset(&bcast, 0, sizeof(bcast));
    bcast.sin_family      = AF_INET;
    bcast.sin_port        = htons(DNS_UDP_PORT);
    inet_pton(AF_INET, BROADCAST_ADDR, &bcast.sin_addr);

    const char *msg = MSG_DISCOVER "\n";
    printf("[web] Broadcasting DNS_DISCOVER to %s:%d ...\n",
           BROADCAST_ADDR, DNS_UDP_PORT);
    sendto(s, msg, (int)strlen(msg), 0,
           (struct sockaddr *)&bcast, sizeof(bcast));

    /* Wait for reply */
    char buf[MAX_MSG_LEN] = {0};
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int n = (int)recvfrom(s, buf, sizeof(buf) - 1, 0,
                          (struct sockaddr *)&from, &fromlen);
    CLOSE_SOCKET(s);

    if (n > 0) {
        buf[n] = '\0';
        char ip[INET_ADDRSTRLEN]; int port;
        if (sscanf(buf, "DNS_HERE %45s %d", ip, &port) == 2) {
            strncpy(dns_ip, ip, INET_ADDRSTRLEN - 1);
            *dns_port = port;
            printf("[web] DNS discovered at %s:%d (via broadcast)\n", ip, port);
            return 1;
        }
    }

    /* ── Fallback: use hard-coded address ──────────────────────────────── */
    printf("[web] No broadcast reply — falling back to %s:%d\n",
           DNS_FALLBACK_IP, DNS_UDP_PORT);
    strncpy(dns_ip, DNS_FALLBACK_IP, INET_ADDRSTRLEN - 1);
    *dns_port = DNS_UDP_PORT;
    return 1; /* still "ok", we have an address to try */
}

/* ─── DNS registration ───────────────────────────────────────────────────── */

static void get_my_ip(char *out, size_t outlen)
{
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_INVALID(s)) { strncpy(out, "127.0.0.1", outlen - 1); return; }
    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port   = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);
    if (connect(s, (struct sockaddr *)&remote, sizeof(remote)) == 0) {
        struct sockaddr_in local; socklen_t len = sizeof(local);
        if (getsockname(s, (struct sockaddr *)&local, &len) == 0) {
            inet_ntop(AF_INET, &local.sin_addr, out, (socklen_t)outlen);
            CLOSE_SOCKET(s); return;
        }
    }
    CLOSE_SOCKET(s);
    strncpy(out, "127.0.0.1", outlen - 1);
}

static void register_with_dns(const char *dns_ip, int dns_port,
                               const char *name,   const char *my_ip, int my_port)
{
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_INVALID(s)) { fprintf(stderr, "[web] Cannot create UDP socket\n"); return; }

    sock_set_timeout(s, DISCOVER_TIMEOUT_MS);

    struct sockaddr_in dns_addr;
    memset(&dns_addr, 0, sizeof(dns_addr));
    dns_addr.sin_family = AF_INET;
    dns_addr.sin_port   = htons(dns_port);
    inet_pton(AF_INET, dns_ip, &dns_addr.sin_addr);

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), "%s %s %s %d\n",
             MSG_REGISTER, name, my_ip, my_port);
    printf("[web] Registering: \"%.*s\"\n", (int)strlen(msg) - 1, msg);
    sendto(s, msg, (int)strlen(msg), 0,
           (struct sockaddr *)&dns_addr, sizeof(dns_addr));

    /* Read acknowledgement (optional, don't abort if missing) */
    char buf[MAX_MSG_LEN] = {0};
    int n = (int)recv(s, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        printf("[web] DNS replied: %s", buf);
    }
    CLOSE_SOCKET(s);
}

/* ─── HTTP request handler ───────────────────────────────────────────────── */

static void handle_client(sock_t client_sock, const char *site_name)
{
    char req[1024] = {0};
    int n = (int)recv(client_sock, req, sizeof(req) - 1, 0);
    if (n > 0) {
        req[n] = '\0';
        /* Print first line of request */
        char *end = strchr(req, '\r');
        if (!end) end = strchr(req, '\n');
        if (end) *end = '\0';
        printf("[web] Request: %s  (site: %s)\n", req, site_name);
    }
    send(client_sock, HTML_PAGE, (int)strlen(HTML_PAGE), 0);
    CLOSE_SOCKET(client_sock);
}

/* ─── Main ───────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    const char *site_name = (argc > 1) ? argv[1] : "meusite.local";
    int         my_port   = (argc > 2) ? atoi(argv[2]) : WEB_DEFAULT_PORT;

    if (net_init() != 0) return 1;

    /* ── 1. Bind TCP socket ─────────────────────────────────────────────── */
    sock_t server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (IS_INVALID(server_sock)) {
        fprintf(stderr, "[web] socket() failed: %d\n", SOCK_ERR);
        net_cleanup(); return 1;
    }
    int yes = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family      = AF_INET;
    bind_addr.sin_port        = htons((unsigned short)my_port);
    bind_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "[web] bind() failed: %d\n", SOCK_ERR);
        CLOSE_SOCKET(server_sock); net_cleanup(); return 1;
    }
    if (listen(server_sock, 8) == SOCKET_ERROR) {
        fprintf(stderr, "[web] listen() failed: %d\n", SOCK_ERR);
        CLOSE_SOCKET(server_sock); net_cleanup(); return 1;
    }

    /* ── 2. Discover my own IP ──────────────────────────────────────────── */
    char my_ip[INET_ADDRSTRLEN] = {0};
    get_my_ip(my_ip, sizeof(my_ip));
    printf("[web] My IP: %s, port: %d, site-name: \"%s\"\n",
           my_ip, my_port, site_name);

    /* ── 3. Discover DNS and register ──────────────────────────────────── */
    char dns_ip[INET_ADDRSTRLEN] = {0};
    int  dns_port = DNS_UDP_PORT;
    discover_dns(dns_ip, &dns_port);
    register_with_dns(dns_ip, dns_port, site_name, my_ip, my_port);

    /* ── 4. Serve HTTP requests ─────────────────────────────────────────── */
    printf("[web] Serving HTTP on port %d — waiting for connections...\n", my_port);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        sock_t client_sock = accept(server_sock,
                                    (struct sockaddr *)&client_addr, &client_len);
        if (IS_INVALID(client_sock)) continue;

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("[web] Connection from %s:%d\n",
               client_ip, ntohs(client_addr.sin_port));

        handle_client(client_sock, site_name);
    }

    CLOSE_SOCKET(server_sock);
    net_cleanup();
    return 0;
}
