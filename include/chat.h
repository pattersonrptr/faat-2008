#ifndef CHAT_H
#define CHAT_H

/* ── Platform abstraction ───────────────────────────────────────────────── */

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
   typedef SOCKET socket_t;
#  define INVALID_SOCK  INVALID_SOCKET
#  define sock_close    closesocket
#  define sock_errno()  WSAGetLastError()
#  ifndef SHUT_RDWR
#    define SHUT_RDWR SD_BOTH
#  endif
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <errno.h>
   typedef int socket_t;
#  define INVALID_SOCK  (-1)
#  define sock_close    close
#  define sock_errno()  errno
#endif

/* ── Constants ──────────────────────────────────────────────────────────── */

#define MAX_MSG_LEN  4096
#define MAX_NAME_LEN 64

/* ── Types ──────────────────────────────────────────────────────────────── */

typedef struct {
    socket_t sock;
    char     username[MAX_NAME_LEN];
} ChatCtx;

/* ── Public API ─────────────────────────────────────────────────────────── */

/* Starts the interactive chat session over an already-connected socket.
   Blocks until the user types /quit or the connection is lost. */
void chat_start(ChatCtx *ctx);

#endif /* CHAT_H */
