#ifndef NET_UTIL_H
#define NET_UTIL_H

#include "chat.h"   /* socket_t, MAX_MSG_LEN */

/* Send exactly len bytes; returns len on success, <=0 on error/disconnect. */
int  net_send_all(socket_t sock, const char *buf, int len);

/* Read a '\n'-terminated line (NUL-terminated result, newline kept).
   Returns bytes stored (>0), 0 on clean disconnect, -1 on error. */
int  net_recv_line(socket_t sock, char *buf, int maxlen);

/* Strip trailing \r and \n in-place. */
void net_strip_crlf(char *s);

#endif /* NET_UTIL_H */
