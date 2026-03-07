/**
 * client.c — Web client for the cdns experiment.
 *
 * Flow:
 *   1. Broadcast DNS_DISCOVER to find the DNS server.
 *      Falls back to DNS_FALLBACK_IP if no broadcast reply.
 *   2. Query DNS: DNS_QUERY <site-name>  → gets IP:port.
 *   3. Connect via TCP to the web server.
 *   4. Send a minimal HTTP GET request.
 *   5. Receive the HTTP response.
 *   6. Render the HTML in the terminal.
 *
 * Usage:  client [site-name]
 *   site-name defaults to "meusite.local"
 */

#include "common.h"

/* ══════════════════════════════════════════════════════════════════════════
 * HTML RENDERER
 * ══════════════════════════════════════════════════════════════════════════
 *
 * Very small subset of HTML:
 *   <h1>…</h1>   → uppercased, padded with ═══ lines
 *   <h2>…</h2>   → uppercased, padded with ─── lines
 *   <h3>…</h3>   → text with "### " prefix
 *   <p>…</p>     → paragraph with leading blank line
 *   <b>…</b>     → surrounded by ** **
 *   <ul>         → start list
 *   <li>…</li>   → "  • " bullet
 *   <br>         → newline
 *   <title>      → shown as "[ Page: … ]"
 *   All other tags are stripped; text nodes are printed as-is.
 */

#define TERM_WIDTH 70

static void print_line(char ch, int width)
{
    for (int i = 0; i < width; i++) putchar(ch);
    putchar('\n');
}

static void print_upper(const char *s)
{
    for (; *s; s++) putchar((char)toupper((unsigned char)*s));
}

/* Copies tag content (between < and >) into dst; returns ptr after '>'. */
static const char *extract_tag(const char *p, char *tag, size_t taglen)
{
    /* p points at '<' */
    p++; /* skip '<' */
    size_t i = 0;
    while (*p && *p != '>' && i < taglen - 1)
        tag[i++] = (char)tolower((unsigned char)*p++);
    tag[i] = '\0';
    if (*p == '>') p++;
    return p;
}

/* Extract text up to the next '<', writing into buf (max buflen-1 chars). */
static const char *extract_text(const char *p, char *buf, size_t buflen)
{
    size_t i = 0;
    while (*p && *p != '<' && i < buflen - 1)
        buf[i++] = *p++;
    buf[i] = '\0';
    return p;
}

/* Decode a handful of HTML entities in-place. */
static void decode_entities(char *s)
{
    static const struct { const char *ent; char rep; } table[] = {
        {"&amp;",  '&'}, {"&lt;",   '<'}, {"&gt;",   '>'},
        {"&quot;", '"'}, {"&apos;", '\''}, {NULL, 0}
    };
    for (int i = 0; table[i].ent; i++) {
        char *pos;
        size_t elen = strlen(table[i].ent);
        while ((pos = strstr(s, table[i].ent)) != NULL) {
            *pos = table[i].rep;
            memmove(pos + 1, pos + elen, strlen(pos + elen) + 1);
        }
    }
}

/*
 * Collect all text content within a block element, handling inline tags
 * like <b>, <strong>, <em>, <a> — renders inline markup as it goes.
 * Stops at the matching closing tag (e.g. </p>, </li>).
 * Returns pointer past the closing tag.
 */
static const char *render_inline(const char *p, const char *close_tag)
{
    char tag[128], text[512];
    while (*p) {
        if (*p == '<') {
            const char *before = p;
            p = extract_tag(p, tag, sizeof(tag));

            /* Stop at the matching close tag */
            if (strcmp(tag, close_tag) == 0)
                return p;

            /* Known inline tags */
            if (strcmp(tag, "b") == 0 || strcmp(tag, "strong") == 0) {
                char ctag[130];
                snprintf(ctag, sizeof(ctag), "/%s", tag);
                printf("**");
                p = render_inline(p, ctag);
                printf("**");
            }
            else if (strcmp(tag, "em") == 0 || strcmp(tag, "i") == 0) {
                char ctag[130];
                snprintf(ctag, sizeof(ctag), "/%s", tag);
                printf("_");
                p = render_inline(p, ctag);
                printf("_");
            }
            else if (strcmp(tag, "br") == 0 || strcmp(tag, "br/") == 0) {
                putchar('\n');
            }
            else if (tag[0] == '/') {
                /* Any other closing tag: stop (we hit something unexpected) */
                return p;
            }
            else {
                /* Opening tag we don't know inline: skip forward */
                (void)before;
            }
        } else {
            p = extract_text(p, text, sizeof(text));
            decode_entities(text);
            printf("%s", text);
        }
    }
    return p;
}

static void render_html(const char *html)
{
    /* Skip HTTP headers */
    const char *body = strstr(html, "\r\n\r\n");
    if (body) body += 4;
    else {
        body = strstr(html, "\n\n");
        if (body) body += 2;
        else body = html;
    }

    /* Find <body> if present — portable case-insensitive search */
    const char *b = NULL;
    {
        const char *tmp = body;
        while (*tmp) {
            if (tolower((unsigned char)tmp[0]) == '<' &&
                tolower((unsigned char)tmp[1]) == 'b' &&
                tolower((unsigned char)tmp[2]) == 'o' &&
                tolower((unsigned char)tmp[3]) == 'd' &&
                tolower((unsigned char)tmp[4]) == 'y') { b = tmp; break; }
            tmp++;
        }
    }
    if (b) {
        const char *gt = strchr(b, '>');
        if (gt) body = gt + 1;
    }

    putchar('\n');
    print_line('=', TERM_WIDTH);
    printf("  RENDERIZACAO HTML\n");
    print_line('=', TERM_WIDTH);
    putchar('\n');

    const char *p = body;
    char tag[128], text[512];
    int in_ul = 0;

    while (*p) {
        if (*p == '<') {
            p = extract_tag(p, tag, sizeof(tag));

            /* ── Block tags ─────────────────────────────────────────── */
            if (strcmp(tag, "h1") == 0) {
                /* collect full content (may have inline tags) */
                char buf[512] = {0};
                /* simple: just grab text until </h1> (h1 rarely has inline) */
                p = extract_text(p, buf, sizeof(buf));
                decode_entities(buf);
                putchar('\n');
                print_line('=', TERM_WIDTH);
                printf("  "); print_upper(buf); putchar('\n');
                print_line('=', TERM_WIDTH);
            }
            else if (strcmp(tag, "h2") == 0) {
                char buf[512] = {0};
                p = extract_text(p, buf, sizeof(buf));
                decode_entities(buf);
                putchar('\n');
                print_line('-', TERM_WIDTH);
                printf("  "); print_upper(buf); putchar('\n');
                print_line('-', TERM_WIDTH);
            }
            else if (strcmp(tag, "h3") == 0) {
                char buf[512] = {0};
                p = extract_text(p, buf, sizeof(buf));
                decode_entities(buf);
                printf("\n### %s\n", buf);
            }
            else if (strcmp(tag, "title") == 0) {
                char buf[256] = {0};
                p = extract_text(p, buf, sizeof(buf));
                decode_entities(buf);
                printf("[ Pagina: %s ]\n", buf);
            }
            else if (strcmp(tag, "p") == 0) {
                /* <p> can contain inline elements like <b> */
                printf("\n  ");
                p = render_inline(p, "/p");
                putchar('\n');
            }
            else if (strcmp(tag, "ul") == 0 || strcmp(tag, "ol") == 0) {
                in_ul = 1; putchar('\n');
            }
            else if (strcmp(tag, "/ul") == 0 || strcmp(tag, "/ol") == 0) {
                in_ul = 0;
            }
            else if (strcmp(tag, "li") == 0) {
                printf("  %s ", in_ul ? "*" : "-");
                p = render_inline(p, "/li");
                putchar('\n');
            }
            else if (strcmp(tag, "br") == 0 || strcmp(tag, "br/") == 0) {
                putchar('\n');
            }
            /* All other tags: ignore */
        } else {
            /* Whitespace / text between block tags — skip silently */
            p = extract_text(p, text, sizeof(text));
            (void)text;
        }
    }

    putchar('\n');
    print_line('=', TERM_WIDTH);
    putchar('\n');
}

/* ══════════════════════════════════════════════════════════════════════════
 * DNS DISCOVERY
 * ══════════════════════════════════════════════════════════════════════════ */

static int discover_dns(char *dns_ip, int *dns_port)
{
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_INVALID(s)) return 0;

    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, (const char *)&yes, sizeof(yes));
    sock_set_timeout(s, DISCOVER_TIMEOUT_MS);

    struct sockaddr_in bcast;
    memset(&bcast, 0, sizeof(bcast));
    bcast.sin_family = AF_INET;
    bcast.sin_port   = htons(DNS_UDP_PORT);
    inet_pton(AF_INET, BROADCAST_ADDR, &bcast.sin_addr);

    const char *msg = MSG_DISCOVER "\n";
    printf("[client] Broadcasting DNS_DISCOVER to %s:%d ...\n",
           BROADCAST_ADDR, DNS_UDP_PORT);
    sendto(s, msg, (int)strlen(msg), 0,
           (struct sockaddr *)&bcast, sizeof(bcast));

    char buf[MAX_MSG_LEN] = {0};
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int n = (int)recvfrom(s, buf, sizeof(buf)-1, 0,
                          (struct sockaddr *)&from, &fromlen);
    CLOSE_SOCKET(s);

    if (n > 0) {
        buf[n] = '\0';
        char ip[INET_ADDRSTRLEN]; int port;
        if (sscanf(buf, "DNS_HERE %45s %d", ip, &port) == 2) {
            strncpy(dns_ip, ip, INET_ADDRSTRLEN - 1);
            *dns_port = port;
            printf("[client] DNS found at %s:%d (via broadcast)\n", ip, port);
            return 1;
        }
    }

    printf("[client] No broadcast reply — falling back to %s:%d\n",
           DNS_FALLBACK_IP, DNS_UDP_PORT);
    strncpy(dns_ip, DNS_FALLBACK_IP, INET_ADDRSTRLEN - 1);
    *dns_port = DNS_UDP_PORT;
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
 * DNS QUERY
 * ══════════════════════════════════════════════════════════════════════════ */

static int query_dns(const char *dns_ip, int dns_port,
                     const char *name,
                     char *out_ip, int *out_port)
{
    sock_t s = socket(AF_INET, SOCK_DGRAM, 0);
    if (IS_INVALID(s)) return 0;

    sock_set_timeout(s, DISCOVER_TIMEOUT_MS);

    struct sockaddr_in dns_addr;
    memset(&dns_addr, 0, sizeof(dns_addr));
    dns_addr.sin_family = AF_INET;
    dns_addr.sin_port   = htons((unsigned short)dns_port);
    inet_pton(AF_INET, dns_ip, &dns_addr.sin_addr);

    char msg[MAX_MSG_LEN];
    snprintf(msg, sizeof(msg), "%s %s\n", MSG_QUERY, name);
    printf("[client] Querying DNS for \"%s\"...\n", name);
    sendto(s, msg, (int)strlen(msg), 0,
           (struct sockaddr *)&dns_addr, sizeof(dns_addr));

    char buf[MAX_MSG_LEN] = {0};
    int n = (int)recv(s, buf, sizeof(buf)-1, 0);
    CLOSE_SOCKET(s);

    if (n <= 0) {
        fprintf(stderr, "[client] No reply from DNS\n");
        return 0;
    }
    buf[n] = '\0';

    if (strncmp(buf, MSG_NXDOMAIN, strlen(MSG_NXDOMAIN)) == 0) {
        fprintf(stderr, "[client] DNS: NXDOMAIN — \"%s\" not found\n", name);
        return 0;
    }

    char ip[INET_ADDRSTRLEN]; int port;
    if (sscanf(buf, "DNS_ANSWER %45s %d", ip, &port) == 2) {
        strncpy(out_ip, ip, INET_ADDRSTRLEN - 1);
        *out_port = port;
        printf("[client] DNS resolved \"%s\" → %s:%d\n", name, ip, port);
        return 1;
    }

    fprintf(stderr, "[client] Unexpected DNS reply: %s", buf);
    return 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 * HTTP GET
 * ══════════════════════════════════════════════════════════════════════════ */

static int http_get(const char *ip, int port, const char *host,
                    char *response, size_t response_len)
{
    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (IS_INVALID(s)) return 0;

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port   = htons((unsigned short)port);
    inet_pton(AF_INET, ip, &srv.sin_addr);

    printf("[client] Connecting to %s:%d ...\n", ip, port);
    if (connect(s, (struct sockaddr *)&srv, sizeof(srv)) == SOCKET_ERROR) {
        fprintf(stderr, "[client] connect() failed: %d\n", SOCK_ERR);
        CLOSE_SOCKET(s); return 0;
    }

    /* Send HTTP request */
    char req[MAX_MSG_LEN];
    snprintf(req, sizeof(req),
             "GET / HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n", host);
    send(s, req, (int)strlen(req), 0);
    printf("[client] Sent HTTP GET /\n");

    /* Receive response */
    size_t total = 0;
    int n;
    while (total < response_len - 1) {
        n = (int)recv(s, response + total, (int)(response_len - 1 - total), 0);
        if (n <= 0) break;
        total += (size_t)n;
    }
    response[total] = '\0';
    CLOSE_SOCKET(s);

    printf("[client] Received %u bytes\n", (unsigned)total);
    return total > 0 ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════════════════
 * MAIN
 * ══════════════════════════════════════════════════════════════════════════ */

int main(int argc, char *argv[])
{
    const char *site_name = (argc > 1) ? argv[1] : "meusite.local";

    if (net_init() != 0) return 1;

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║           CLIENTE CDNS — Experimento DNS + HTTP                 ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");
    printf("[client] Procurando a pagina: \"%s\"\n\n", site_name);

    /* Step 1: Discover DNS */
    printf("=== PASSO 1: Descobrir o servidor DNS ===\n");
    char dns_ip[INET_ADDRSTRLEN] = {0};
    int  dns_port = DNS_UDP_PORT;
    if (!discover_dns(dns_ip, &dns_port)) {
        fprintf(stderr, "[client] Falha ao encontrar DNS\n");
        net_cleanup(); return 1;
    }

    /* Step 2: Query DNS for site */
    printf("\n=== PASSO 2: Resolver nome no DNS ===\n");
    char web_ip[INET_ADDRSTRLEN] = {0};
    int  web_port = 0;
    if (!query_dns(dns_ip, dns_port, site_name, web_ip, &web_port)) {
        fprintf(stderr, "[client] Falha ao resolver \"%s\"\n", site_name);
        net_cleanup(); return 1;
    }

    /* Step 3: HTTP GET */
    printf("\n=== PASSO 3: Buscar pagina HTTP ===\n");
    static char response[MAX_HTML_LEN];
    if (!http_get(web_ip, web_port, site_name, response, sizeof(response))) {
        fprintf(stderr, "[client] Falha ao buscar pagina HTTP\n");
        net_cleanup(); return 1;
    }

    /* Step 4: Render */
    printf("\n=== PASSO 4: Renderizando HTML ===\n");
    render_html(response);

    net_cleanup();
    return 0;
}
