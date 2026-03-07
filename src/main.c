#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chat.h"

/* ── Usage ──────────────────────────────────────────────────────────────── */

static void usage(const char *prog)
{
    fprintf(stderr,
        "cchat — simple terminal chat over TCP\n\n"
        "Usage:\n"
        "  Server : %s server <port> <username>\n"
        "  Client : %s client <host> <port> <username>\n\n"
        "Example (same machine, two terminals):\n"
        "  Terminal 1:  %s server 5000 Alice\n"
        "  Terminal 2:  %s client localhost 5000 Bob\n\n"
        "Commands inside chat:\n"
        "  /quit   disconnect and exit\n",
        prog, prog, prog, prog);
    exit(EXIT_FAILURE);
}

/* ── Server mode ────────────────────────────────────────────────────────── */

static socket_t do_server(const char *port_str)
{
    int port = atoi(port_str);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "[-] Invalid port: %s\n", port_str);
        exit(EXIT_FAILURE);
    }

    socket_t lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock == INVALID_SOCK) {
        fprintf(stderr, "[-] socket() failed: %d\n", sock_errno());
        exit(EXIT_FAILURE);
    }

    /* Allow port reuse so re-running doesn't fail with "address in use" */
    int opt = 1;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)port);

    if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "[-] bind() failed on port %d: %d\n", port, sock_errno());
        sock_close(lsock);
        exit(EXIT_FAILURE);
    }

    listen(lsock, 1);
    printf("[*] Listening on port %d — waiting for a peer...\n", port);

    struct sockaddr_in peer;
    socklen_t peerlen = sizeof(peer);
    socket_t csock = accept(lsock, (struct sockaddr *)&peer, &peerlen);
    sock_close(lsock);   /* done with the listening socket */

    if (csock == INVALID_SOCK) {
        fprintf(stderr, "[-] accept() failed: %d\n", sock_errno());
        exit(EXIT_FAILURE);
    }

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
    printf("[*] Peer connected from %s:%d\n", ip, ntohs(peer.sin_port));

    return csock;
}

/* ── Client mode ────────────────────────────────────────────────────────── */

static socket_t do_client(const char *host, const char *port_str)
{
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int rc = getaddrinfo(host, port_str, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "[-] getaddrinfo(%s:%s) failed: error %d\n",
                host, port_str, rc);
        exit(EXIT_FAILURE);
    }

    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCK) {
        fprintf(stderr, "[-] socket() failed: %d\n", sock_errno());
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    printf("[*] Connecting to %s:%s...\n", host, port_str);
    if (connect(sock, res->ai_addr, (socklen_t)res->ai_addrlen) < 0) {
        fprintf(stderr, "[-] connect() failed — is the server running? (error %d)\n",
                sock_errno());
        sock_close(sock);
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

    return sock;
}

/* ── Entry point ────────────────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[-] WSAStartup failed\n");
        return EXIT_FAILURE;
    }
#endif

    if (argc < 2) usage(argv[0]);

    ChatCtx ctx;
    memset(&ctx, 0, sizeof(ctx));

    if (strcmp(argv[1], "server") == 0) {
        if (argc < 4) usage(argv[0]);
        strncpy(ctx.username, argv[3], MAX_NAME_LEN - 1);
        ctx.sock = do_server(argv[2]);

    } else if (strcmp(argv[1], "client") == 0) {
        if (argc < 5) usage(argv[0]);
        strncpy(ctx.username, argv[4], MAX_NAME_LEN - 1);
        ctx.sock = do_client(argv[2], argv[3]);

    } else {
        usage(argv[0]);
    }

    chat_start(&ctx);

    sock_close(ctx.sock);

#ifdef _WIN32
    WSACleanup();
#endif
    return EXIT_SUCCESS;
}
