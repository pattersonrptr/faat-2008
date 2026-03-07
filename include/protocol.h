#ifndef PROTOCOL_H
#define PROTOCOL_H

/* ── Default ports ──────────────────────────────────────────────────────── */

#define DNS_DEFAULT_PORT    5300
#define RELAY_DEFAULT_PORT  5000

/* ── Service name registered with the DNS server ────────────────────────── */

#define SERVICE_NAME  "cchat"

/* ── DNS protocol (UDP, lines terminated with '\n') ────────────────────── */
/*
 *  Client/relay → DNS:
 *    QUERY cchat\n
 *    REGISTER cchat 192.168.1.10 5000\n
 *
 *  DNS → client/relay:
 *    OK 192.168.1.10:5000\n   (QUERY hit)
 *    OK\n                      (REGISTER ack)
 *    ERR <reason>\n            (on failure)
 */

/* ── Relay protocol (TCP, lines terminated with '\n') ──────────────────── */
/*
 *  Client → server (first message, handshake):
 *    HELLO <username>\n
 *
 *  Client → server (chat message):
 *    <raw text>\n
 *
 *  Server → all other clients (broadcast):
 *    <username>: <raw text>\n
 *
 *  Server → all clients (system notification):
 *    *** <username> joined\n
 *    *** <username> left\n
 */

#define DNS_BUF_SIZE  256
#define MAX_CLIENTS   32

#endif /* PROTOCOL_H */
