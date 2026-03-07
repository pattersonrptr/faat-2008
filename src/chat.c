#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chat.h"
#include "net_util.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#endif

/* ── Receive thread ─────────────────────────────────────────────────────── */

/*
 * Runs in a background thread.  Reads lines from the socket and prints them.
 * When the connection closes, it notifies the user and exits.
 */
#ifdef _WIN32
static DWORD WINAPI recv_thread_fn(LPVOID arg)
#else
static void *recv_thread_fn(void *arg)
#endif
{
    ChatCtx *ctx = (ChatCtx *)arg;
    char buf[MAX_MSG_LEN];

    for (;;) {
        int n = net_recv_line(ctx->sock, buf, (int)sizeof(buf));
        if (n <= 0) {
            /* \r clears the "> " prompt that may be on the current line */
            printf("\r[*] Connection closed by peer. Press Enter to exit.\n");
            fflush(stdout);
            break;
        }
        net_strip_crlf(buf);
        /* Overwrite any partial input the user may have on-screen, then
           reprint the prompt so they can continue typing. */
        printf("\r%s\n> ", buf);
        fflush(stdout);
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/* ── Public chat loop ───────────────────────────────────────────────────── */

void chat_start(ChatCtx *ctx)
{
    printf("[*] Chat started!  Type a message and press Enter.\n"
           "[*] Use /quit to disconnect.\n\n");

    /* ── Start the receive thread ── */
#ifdef _WIN32
    HANDLE hThread = CreateThread(NULL, 0, recv_thread_fn, ctx, 0, NULL);
    if (!hThread) {
        fprintf(stderr, "[-] CreateThread failed: %lu\n", GetLastError());
        return;
    }
#else
    pthread_t tid;
    if (pthread_create(&tid, NULL, recv_thread_fn, ctx) != 0) {
        perror("[-] pthread_create");
        return;
    }
    pthread_detach(tid);
#endif

    /* ── Main thread: read stdin and send ── */
    char input[MAX_MSG_LEN];
    char outbuf[MAX_MSG_LEN + MAX_NAME_LEN + 4]; /* "NAME: msg\n" */

    for (;;) {
        printf("> ");
        fflush(stdout);

        if (!fgets(input, (int)sizeof(input), stdin)) break;
        net_strip_crlf(input);

        if (input[0] == '\0') continue;   /* blank line, ignore */

        if (strcmp(input, "/quit") == 0) {
            printf("[*] Disconnecting...\n");
            break;
        }

        int len = snprintf(outbuf, sizeof(outbuf), "%s: %s\n",
                           ctx->username, input);

        if (net_send_all(ctx->sock, outbuf, len) <= 0) {
            printf("[*] Send failed — connection lost.\n");
            break;
        }
    }

    /* Signal the peer that we are done, and let the receive thread exit. */
    shutdown(ctx->sock, SHUT_RDWR);

#ifdef _WIN32
    WaitForSingleObject(hThread, 3000);
    CloseHandle(hThread);
#endif
}
