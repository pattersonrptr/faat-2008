#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* =========================================================
 *  Strings
 * ========================================================= */
void str_trim(char *s) {
    /* Remove espaços à direita */
    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    /* Remove espaços à esquerda */
    int start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start > 0) {
        memmove(s, s + start, strlen(s + start) + 1);
    }
}

void str_upper(char *s) {
    while (*s) { *s = (char)toupper((unsigned char)*s); s++; }
}

void str_lower(char *s) {
    while (*s) { *s = (char)tolower((unsigned char)*s); s++; }
}

int str_contains(const char *haystack, const char *needle) {
    /* Busca case-insensitive simples */
    char h[512], n[256];
    strncpy(h, haystack, sizeof(h) - 1); h[sizeof(h)-1] = '\0';
    strncpy(n, needle,   sizeof(n) - 1); n[sizeof(n)-1] = '\0';
    str_lower(h);
    str_lower(n);
    return strstr(h, n) != NULL;
}

/* =========================================================
 *  Entrada segura de dados
 * ========================================================= */
void input_string(const char *prompt, char *buf, int max_len) {
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, max_len, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }
    /* Remove newline final */
    int len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    str_trim(buf);
}

int input_int(const char *prompt, int min, int max) {
    char buf[32];
    int  val;
    while (1) {
        printf("%s [%d-%d]: ", prompt, min, max);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            /* EOF — retorna o valor mínimo para não travar */
            return min;
        }
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max) {
            return val;
        }
        printf("  Valor invalido. Informe um numero entre %d e %d.\n", min, max);
    }
}

double input_double(const char *prompt, double min, double max) {
    char   buf[32];
    double val;
    while (1) {
        printf("%s [%.2f-%.2f]: ", prompt, min, max);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) {
            return min;
        }
        if (sscanf(buf, "%lf", &val) == 1 && val >= min && val <= max) {
            return val;
        }
        printf("  Valor invalido.\n");
    }
}

int input_confirm(const char *prompt) {
    char buf[8];
    while (1) {
        printf("%s [s/n]: ", prompt);
        fflush(stdout);
        if (fgets(buf, sizeof(buf), stdin) == NULL) return 0;
        char c = (char)tolower((unsigned char)buf[0]);
        if (c == 's') return 1;
        if (c == 'n') return 0;
        printf("  Digite 's' para sim ou 'n' para nao.\n");
    }
}

/* =========================================================
 *  Datas
 * ========================================================= */
time_t date_from_string(const char *s) {
    struct tm t;
    memset(&t, 0, sizeof(t));
    if (sscanf(s, "%d/%d/%d", &t.tm_mday, &t.tm_mon, &t.tm_year) != 3) {
        return (time_t)-1;
    }
    t.tm_mon  -= 1;       /* tm_mon: 0-11 */
    t.tm_year -= 1900;    /* tm_year: anos desde 1900 */
    t.tm_hour = 12;       /* meio-dia para evitar problemas de DST */
    t.tm_isdst = -1;
    return mktime(&t);
}

void date_to_string(time_t t, char *buf) {
    struct tm *tm_info = localtime(&t);
    if (tm_info == NULL) {
        strcpy(buf, "00/00/0000");
        return;
    }
    sprintf(buf, "%02d/%02d/%04d",
            tm_info->tm_mday,
            tm_info->tm_mon + 1,
            tm_info->tm_year + 1900);
}

void datetime_to_string(time_t t, char *buf) {
    struct tm *tm_info = localtime(&t);
    if (tm_info == NULL) {
        strcpy(buf, "00/00/0000 00:00");
        return;
    }
    sprintf(buf, "%02d/%02d/%04d %02d:%02d",
            tm_info->tm_mday,
            tm_info->tm_mon + 1,
            tm_info->tm_year + 1900,
            tm_info->tm_hour,
            tm_info->tm_min);
}

int date_diff_days(time_t start, time_t end) {
    double diff = difftime(end, start);
    return (int)(diff / 86400.0);
}

int date_is_valid(const char *s) {
    int d, m, y;
    if (sscanf(s, "%d/%d/%d", &d, &m, &y) != 3) return 0;
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > 31) return 0;
    if (y < 1900 || y > 2200) return 0;
    /* Dias por mês (simplificado, considera bissexto) */
    int days_in_month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) days_in_month[2] = 29;
    return d <= days_in_month[m];
}

time_t date_today(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    t->tm_hour = 0; t->tm_min = 0; t->tm_sec = 0;
    t->tm_isdst = -1;
    return mktime(t);
}

/* =========================================================
 *  Hash DJB2
 * ========================================================= */
unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++) != 0) {
        hash = ((hash << 5) + hash) + (unsigned long)c;
    }
    return hash;
}

void hash_to_hex(unsigned long hash, char *buf) {
    sprintf(buf, "%016lx", hash);
}

/* =========================================================
 *  Geração de código de confirmação
 * ========================================================= */
void generate_confirmation_code(char *buf) {
    /* Formato: "RES-XXXXXX" onde X é alfanumérico maiúsculo */
    static const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    int charset_len = (int)(sizeof(charset) - 1);
    srand((unsigned int)time(NULL) ^ (unsigned int)(size_t)buf);
    buf[0] = 'R'; buf[1] = 'E'; buf[2] = 'S'; buf[3] = '-';
    for (int i = 4; i < 10; i++) {
        buf[i] = charset[rand() % charset_len];
    }
    buf[10] = '\0';
}

/* =========================================================
 *  Console / UI
 * ========================================================= */
void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause_screen(void) {
    printf("\n  Pressione Enter para continuar...");
    fflush(stdout);
    /* Consome qualquer caractere até '\n' */
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void print_separator(char c, int width) {
    for (int i = 0; i < width; i++) putchar(c);
    putchar('\n');
}

void print_header(const char *title) {
    print_separator('=', 60);
    int len = (int)strlen(title);
    int pad = (60 - len) / 2;
    printf("%*s%s\n", pad, "", title);
    print_separator('=', 60);
}

/* =========================================================
 *  Validação de CPF
 * ========================================================= */
/* Remove pontuação e verifica dígitos verificadores */
static void cpf_digits_only(const char *in, char *out) {
    int j = 0;
    for (int i = 0; in[i] && j < 11; i++) {
        if (isdigit((unsigned char)in[i])) out[j++] = in[i];
    }
    out[j] = '\0';
}

int cpf_is_valid(const char *cpf) {
    char d[12];
    cpf_digits_only(cpf, d);
    if (strlen(d) != 11) return 0;

    /* Rejeita sequências triviais (111.111.111-11, etc.) */
    int all_same = 1;
    for (int i = 1; i < 11; i++) { if (d[i] != d[0]) { all_same = 0; break; } }
    if (all_same) return 0;

    /* Primeiro dígito verificador */
    int sum = 0;
    for (int i = 0; i < 9; i++) sum += (d[i] - '0') * (10 - i);
    int r1 = (sum * 10) % 11;
    if (r1 == 10) r1 = 0;
    if (r1 != (d[9] - '0')) return 0;

    /* Segundo dígito verificador */
    sum = 0;
    for (int i = 0; i < 10; i++) sum += (d[i] - '0') * (11 - i);
    int r2 = (sum * 10) % 11;
    if (r2 == 10) r2 = 0;
    return r2 == (d[10] - '0');
}

void cpf_format(const char *raw, char *buf) {
    char d[12];
    cpf_digits_only(raw, d);
    if (strlen(d) == 11) {
        sprintf(buf, "%c%c%c.%c%c%c.%c%c%c-%c%c",
                d[0],d[1],d[2], d[3],d[4],d[5],
                d[6],d[7],d[8], d[9],d[10]);
    } else {
        strncpy(buf, raw, 20);
    }
}
