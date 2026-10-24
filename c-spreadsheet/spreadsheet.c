/* --------------------------------------------------------------
 *  Mini‑planilha em C – versão sem flicker + interface retro
 *  Compilação (MinGW):  gcc -Wall -O2 -o spreadsheet.exe spreadsheet.c
 * -------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <conio.h>
#include <windows.h>

/* ---------- Configurações ---------- */
#define ROWS        20
#define COLS        10
#define CELL_WIDTH  12
#define MAX_TEXT    256

/* ---------- Estado da planilha ---------- */
char ***cells;
int cur_row = 0, cur_col = 0;
int edit_mode = 0;
char edit_buffer[MAX_TEXT];
int  edit_len;

/* ---------- Constantes de tela ---------- */
#define SCREEN_WIDTH   ((COLS)*(CELL_WIDTH+1) + 5)
#define SCREEN_HEIGHT  (ROWS + 8)

CHAR_INFO screen_buffer[SCREEN_HEIGHT][SCREEN_WIDTH];

/* ---------- Protótipos necessários ---------- */
double get_cell_value(int row, int col);
double eval_expr(const char *expr);

/* ---------- Parser – declarações antecipadas ---------- */
typedef struct { const char *s; int pos; } ParseState;

/* protótipos de todas as funções de análise */
double parse_expression(ParseState *ps);
double parse_term(ParseState *ps);
double parse_factor(ParseState *ps);
double parse_primary(ParseState *ps);
double parse_number(ParseState *ps);
double parse_cell_reference(ParseState *ps);
void    skip_spaces(ParseState *ps);

/* ---------- Funções auxiliares de console ---------- */
void clear_screen_buffer(void) {
    for (int y = 0; y < SCREEN_HEIGHT; ++y)
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            screen_buffer[y][x].Char.AsciiChar = ' ';
            screen_buffer[y][x].Attributes    = 0x07;
        }
}
void write_char(int x, int y, char ch, WORD attr) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    screen_buffer[y][x].Char.AsciiChar = ch;
    screen_buffer[y][x].Attributes    = attr;
}
void flush_screen(void) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD     bufSize = { SCREEN_WIDTH, SCREEN_HEIGHT };
    COORD     bufCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1 };
    WriteConsoleOutputA(hOut, (CHAR_INFO*)screen_buffer,
                        bufSize, bufCoord, &writeRegion);
}

/* ---------- Inicialização / limpeza ---------- */
void init_cells(void) {
    cells = (char ***)malloc(ROWS * sizeof(char **));
    for (int i = 0; i < ROWS; ++i) {
        cells[i] = (char **)malloc(COLS * sizeof(char *));
        for (int j = 0; j < COLS; ++j) {
            cells[i][j] = (char *)malloc(MAX_TEXT);
            cells[i][j][0] = '\0';
        }
    }
}
void free_cells(void) {
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) free(cells[i][j]);
        free(cells[i]);
    }
    free(cells);
}

/* ---------- Conversão de referências ---------- */
int col_name_to_index(const char *col_name) {
    int idx = toupper(col_name[0]) - 'A';
    return (idx >= 0 && idx < COLS) ? idx : -1;
}
int parse_cell_ref(const char *ref, int *row, int *col) {
    if (!isalpha(ref[0])) return 0;
    *col = col_name_to_index(ref);
    if (*col == -1) return 0;
    *row = 0;
    int i = 1;
    while (isdigit(ref[i])) {
        *row = *row * 10 + (ref[i] - '0');
        ++i;
    }
    (*row)--;                     // 1‑based → 0‑based
    return (ref[i] == '\0' && *row >= 0 && *row < ROWS);
}

/* ---------- Parser de expressões ---------- */
void skip_spaces(ParseState *ps) {
    while (ps->s[ps->pos] && isspace(ps->s[ps->pos])) ++ps->pos;
}
double parse_number(ParseState *ps) {
    skip_spaces(ps);
    char *end;
    double v = strtod(ps->s + ps->pos, &end);
    if (end == ps->s + ps->pos) return NAN;
    ps->pos = (int)(end - ps->s);
    return v;
}
double parse_cell_reference(ParseState *ps) {
    int start = ps->pos;
    if (!isalpha(ps->s[start])) return NAN;
    ++ps->pos;
    while (isalnum(ps->s[ps->pos])) ++ps->pos;
    char ref[10];
    strncpy(ref, ps->s + start, ps->pos - start);
    ref[ps->pos - start] = '\0';
    int r, c;
    return parse_cell_ref(ref, &r, &c) ? get_cell_value(r, c) : NAN;
}
double parse_primary(ParseState *ps) {
    skip_spaces(ps);
    if (ps->s[ps->pos] == '(') {
        ++ps->pos;
        double v = parse_expression(ps);
        skip_spaces(ps);
        if (ps->s[ps->pos] == ')') ++ps->pos;
        else return NAN;
        return v;
    } else if (isdigit(ps->s[ps->pos]) || ps->s[ps->pos] == '.') {
        return parse_number(ps);
    } else if (isalpha(ps->s[ps->pos])) {
        return parse_cell_reference(ps);
    }
    return NAN;
}
double parse_factor(ParseState *ps) {
    double v = parse_primary(ps);
    if (isnan(v)) return NAN;
    while (1) {
        skip_spaces(ps);
        char op = ps->s[ps->pos];
        if (op == '*') {
            ++ps->pos;
            double r = parse_primary(ps);
            if (isnan(r)) return NAN;
            v *= r;
        } else if (op == '/') {
            ++ps->pos;
            double r = parse_primary(ps);
            if (isnan(r) || r == 0) return NAN;
            v /= r;
        } else break;
    }
    return v;
}
double parse_term(ParseState *ps) {
    double v = parse_factor(ps);
    if (isnan(v)) return NAN;
    while (1) {
        skip_spaces(ps);
        char op = ps->s[ps->pos];
        if (op == '+') {
            ++ps->pos;
            double r = parse_factor(ps);
            if (isnan(r)) return NAN;
            v += r;
        } else if (op == '-') {
            ++ps->pos;
            double r = parse_factor(ps);
            if (isnan(r)) return NAN;
            v -= r;
        } else break;
    }
    return v;
}
double parse_expression(ParseState *ps) { return parse_term(ps); }

double eval_expr(const char *expr) {
    ParseState ps = { expr, 0 };
    double r = parse_expression(&ps);
    skip_spaces(&ps);
    return (ps.s[ps.pos] == '\0' && !isnan(r)) ? r : NAN;
}

/* ---------- Valor e texto a exibir ---------- */
double get_cell_value(int row, int col) {
    char *c = cells[row][col];
    if (!c[0]) return 0.0;
    if (c[0] == '=') return eval_expr(c + 1);
    char *end;
    double v = strtod(c, &end);
    return (*end == '\0') ? v : 0.0;
}
void get_display_text(int row, int col, char *out, int max_len) {
    char *c = cells[row][col];
    if (c[0] == '=') {
        double v = get_cell_value(row, col);
        if (isnan(v)) snprintf(out, max_len, "#ERR");
        else if (fabs(v - (int)v) < 1e-9) snprintf(out, max_len, "%d", (int)v);
        else snprintf(out, max_len, "%.2f", v);
    } else {
        strncpy(out, c, max_len - 1);
        out[max_len - 1] = '\0';
    }
}

/* ---------- Desenho da grade (caracteres CP437) ---------- */
void draw_header(void) {
    int y = 1;
    write_char(4, y, 0xC9, 0x07);
    for (int j = 0; j < COLS; ++j) {
        for (int k = 0; k < CELL_WIDTH; ++k)
            write_char(5 + j*(CELL_WIDTH+1) + k, y, 0xCD, 0x07);
        write_char(5 + j*(CELL_WIDTH+1) + CELL_WIDTH, y,
                   (j==COLS-1)?0xBB:0xC9, 0x07);
    }
    y = 2;
    for (int j = 0; j < COLS; ++j) {
        const char col = 'A' + j;
        int base = 5 + j*(CELL_WIDTH+1);
        for (int k = 0; k < CELL_WIDTH; ++k) write_char(base + k, y, ' ', 0x07);
        write_char(base + CELL_WIDTH/2, y, col, 0x07);
    }
}
void draw_borders(void) {
    for (int r = 0; r <= ROWS; ++r) {
        int y = r + 3;
        for (int c = 0; c <= COLS; ++c) {
            int x = c * (CELL_WIDTH + 1) + 4;
            if (r==0 && c==0)               write_char(x, y, 0xC9,0x07);
            else if (r==0 && c==COLS)       write_char(x, y, 0xBB,0x07);
            else if (r==ROWS && c==0)       write_char(x, y, 0xC8,0x07);
            else if (r==ROWS && c==COLS)    write_char(x, y, 0xBC,0x07);
            else if (r==0)                  write_char(x, y, 0xCB,0x07);
            else if (r==ROWS)               write_char(x, y, 0xC7,0x07);
            else if (c==0)                  write_char(x, y, 0xC5,0x07);
            else if (c==COLS)               write_char(x, y, 0xB6,0x07);
            else                            write_char(x, y, 0xCD,0x07);
        }
        for (int c = 0; c < COLS; ++c)
            for (int k = 0; k < CELL_WIDTH; ++k)
                write_char(5 + c*(CELL_WIDTH+1) + k, y, 0xCD, 0x07);
    }
    for (int r = 0; r <= ROWS; ++r) {
        int y = r + 3;
        for (int c = 0; c <= COLS; ++c) {
            int x = c * (CELL_WIDTH + 1) + 4;
            write_char(x, y, 0xBA, 0x07);
        }
    }
}
void draw_cell(int row, int col, int active, int editing) {
    char display[MAX_TEXT];
    if (active && editing) {
        strncpy(display, edit_buffer, CELL_WIDTH);
        display[CELL_WIDTH] = '\0';
    } else {
        get_display_text(row, col, display, CELL_WIDTH + 1);
    }
    WORD attr = active
        ? (BACKGROUND_BLUE |
           FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE |
           FOREGROUND_INTENSITY)   // texto branco forte
        : 0x07;
    int x = col * (CELL_WIDTH + 1) + 5;
    int y = row + 4;
    for (int i = 0; i < CELL_WIDTH; ++i) {
        char ch = (i < (int)strlen(display)) ? display[i] : ' ';
        write_char(x + i, y, ch, attr);
    }
}
void refresh_grid(void) {
    static int first = 1;
    if (first) {
        draw_header();
        draw_borders();
        first = 0;
    }
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            draw_cell(r, c, (r==cur_row && c==cur_col), edit_mode);

    const char *m1 = "Setas: mover | Enter: editar | Delete: limpar | Esc: sair";
    const char *m2 = "Formulas: comece com '=' (ex: =A1+B2)";
    const char *m3 = "Modo edicao: digite texto, Enter confirma, Esc cancela";
    for (int i = 0; i < (int)strlen(m1); ++i) write_char(i, ROWS+5, m1[i], 0x07);
    for (int i = 0; i < (int)strlen(m2); ++i) write_char(i, ROWS+6, m2[i], 0x07);
    for (int i = 0; i < (int)strlen(m3); ++i) write_char(i, ROWS+7, m3[i], 0x07);
    flush_screen();
}

/* ---------- Edição ---------- */
void start_edit(void) {
    edit_mode = 1;
    strcpy(edit_buffer, cells[cur_row][cur_col]);
    edit_len = (int)strlen(edit_buffer);
    refresh_grid();
}
void cancel_edit(void) { edit_mode = 0; refresh_grid(); }
void confirm_edit(void) {
    strcpy(cells[cur_row][cur_col], edit_buffer);
    edit_mode = 0;
    refresh_grid();
}
void delete_cell(void) {
    cells[cur_row][cur_col][0] = '\0';
    refresh_grid();
}

/* ---------- Loop principal ---------- */
int main(void) {
    init_cells();
    clear_screen_buffer();
    refresh_grid();

    while (1) {
        int key = getch();

        if (!edit_mode) {
            if (key == 224) {                   // setas / DEL
                int ext = getch();
                switch (ext) {
                case 72: if (cur_row > 0)    cur_row--; break;   // ↑
                case 80: if (cur_row < ROWS-1) cur_row++; break; // ↓
                case 75: if (cur_col > 0)    cur_col--; break;   // ←
                case 77: if (cur_col < COLS-1) cur_col++; break; // →
                case 83: delete_cell(); break;                     // DEL
                }
            } else {
                switch (key) {
                case 13: start_edit(); break;      // ENTER
                case 27: free_cells(); return 0;   // ESC → sair
                case 8:  delete_cell(); break;     // BACKSPACE
                }
            }
            refresh_grid();           // apenas a célula ativa mudou
        } else {
            if (key == 13)               confirm_edit();
            else if (key == 27)          cancel_edit();
            else if (key == 8) {         // backspace
                if (edit_len > 0) {
                    edit_len--;
                    edit_buffer[edit_len] = '\0';
                }
            } else if (key >= 32 && key <= 126) {   // caractere imprimível
                if (edit_len < MAX_TEXT-1) {
                    edit_buffer[edit_len++] = (char)key;
                    edit_buffer[edit_len]   = '\0';
                }
            }
            draw_cell(cur_row, cur_col, 1, 1);   // redesenha só a célula editada
            flush_screen();
        }
    }
    return 0;   // nunca alcançado
}
