#ifndef UTILS_H
#define UTILS_H

#include <time.h>

/* ── Strings ─────────────────────────────────────────── */
void  str_trim(char *s);                        /* remove espaços nas bordas (in-place) */
void  str_upper(char *s);
void  str_lower(char *s);
int   str_contains(const char *haystack, const char *needle); /* case-insensitive */

/* ── Entrada segura de dados ─────────────────────────── */
void  input_string(const char *prompt, char *buf, int max_len);
int   input_int(const char *prompt, int min, int max);
double input_double(const char *prompt, double min, double max);
int   input_confirm(const char *prompt);        /* retorna 1=sim, 0=não */

/* ── Datas ───────────────────────────────────────────── */
time_t date_from_string(const char *s);         /* "DD/MM/AAAA" → time_t */
void   date_to_string(time_t t, char *buf);     /* time_t → "DD/MM/AAAA" */
void   datetime_to_string(time_t t, char *buf); /* time_t → "DD/MM/AAAA HH:MM" */
int    date_diff_days(time_t start, time_t end);
int    date_is_valid(const char *s);            /* valida "DD/MM/AAAA" */
time_t date_today(void);                        /* meia-noite de hoje */

/* ── Hash ────────────────────────────────────────────── */
unsigned long hash_djb2(const char *str);
void   hash_to_hex(unsigned long hash, char *buf); /* buf deve ter >= 17 chars */

/* ── Geração de código ───────────────────────────────── */
void   generate_confirmation_code(char *buf);   /* ex: "RES-A3F7K2" (12 chars) */

/* ── Console / UI ────────────────────────────────────── */
void   clear_screen(void);
void   pause_screen(void);                      /* "Pressione Enter para continuar..." */
void   print_separator(char c, int width);
void   print_header(const char *title);

/* ── Documentos ─────────────────────────────────────── */
int    cpf_is_valid(const char *cpf);           /* valida CPF (dígitos verificadores) */
void   cpf_format(const char *raw, char *buf);  /* "12345678901" → "123.456.789-01" */

#endif /* UTILS_H */
