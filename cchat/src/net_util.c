#include <string.h>
#include "net_util.h"

int net_send_all(socket_t sock, const char *buf, int len)
{
    int sent = 0;
    while (sent < len) {
        int n = send(sock, buf + sent, len - sent, 0);
        if (n <= 0) return n;
        sent += n;
    }
    return sent;
}

int net_recv_line(socket_t sock, char *buf, int maxlen)
{
    int total = 0;
    while (total < maxlen - 1) {
        char c;
        int n = recv(sock, &c, 1, 0);
        if (n <= 0) return n;
        buf[total++] = c;
        if (c == '\n') break;
    }
    buf[total] = '\0';
    return total;
}

void net_strip_crlf(char *s)
{
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}
