#include "rooms.h"
#include "storage.h"
#include "auth.h"
#include "utils.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* =========================================================
 *  Helpers de conversão
 * ========================================================= */
const char *room_type_str(RoomType t) {
    switch (t) {
        case ROOM_SOLTEIRO:      return "Solteiro";
        case ROOM_DUPLO:         return "Duplo";
        case ROOM_SUITE:         return "Suite";
        case ROOM_PRESIDENCIAL:  return "Presidencial";
        default:                 return "?";
    }
}

const char *room_status_str(RoomStatus s) {
    switch (s) {
        case ROOM_LIVRE:         return "LIVRE";
        case ROOM_OCUPADO:       return "OCUPADO";
        case ROOM_RESERVADO:     return "RESERVADO";
        case ROOM_MANUTENCAO:    return "MANUTENCAO";
        case ROOM_BLOQUEADO:     return "BLOQUEADO";
        default:                 return "?";
    }
}

const char *room_clean_str(CleanStatus s) {
    switch (s) {
        case CLEAN_SUJO:          return "Sujo";
        case CLEAN_EM_LIMPEZA:    return "Em limpeza";
        case CLEAN_LIMPO:         return "Limpo";
        case CLEAN_INSPECIONADO:  return "Inspecionado";
        default:                  return "?";
    }
}

void room_features_str(int features, char *buf, int buf_size) {
    char tmp[128] = "";
    if (features & FEAT_AC)        strcat(tmp, "AC ");
    if (features & FEAT_FRIGOBAR)  strcat(tmp, "Frigobar ");
    if (features & FEAT_BANHEIRA)  strcat(tmp, "Banheira ");
    if (features & FEAT_VARANDA)   strcat(tmp, "Varanda ");
    if (features & FEAT_ACESSIVEL) strcat(tmp, "Acessivel");
    if (tmp[0] == '\0')            strcat(tmp, "Nenhuma");
    strncpy(buf, tmp, (size_t)(buf_size - 1));
    buf[buf_size - 1] = '\0';
}

/* =========================================================
 *  Busca
 * ========================================================= */
Room *room_find_by_id(int id) {
    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].id == id) return &g_rooms[i];
    }
    return NULL;
}

/* Número deve estar em maiúsculo antes de chamar */
Room *room_find_by_number(const char *number) {
    for (int i = 0; i < g_room_count; i++) {
        if (strcmp(g_rooms[i].number, number) == 0) return &g_rooms[i];
    }
    return NULL;
}

/* =========================================================
 *  Exibição
 * ========================================================= */
static void print_table_header(void) {
    printf("  %-4s %-6s %-5s %-12s %-4s %-9s %-12s %-14s\n",
           "ID", "Num.", "Andar", "Tipo", "Cap.", "Diaria",
           "Status", "Limpeza");
    print_separator('-', 74);
}

void room_print_brief(const Room *r) {
    printf("  %-4d %-6s %-5d %-12s %-4d R$%-7.2f %-12s %-14s\n",
           r->id, r->number, r->floor,
           room_type_str(r->type), r->capacity,
           r->daily_rate,
           room_status_str(r->status),
           room_clean_str(r->clean_status));
}

void room_print_full(const Room *r) {
    char feat[128];
    room_features_str(r->features, feat, sizeof(feat));
    print_separator('-', 60);
    printf("  Quarto #%s  (ID: %d)\n", r->number, r->id);
    print_separator('-', 60);
    printf("  Andar       : %d\n",       r->floor);
    printf("  Tipo        : %s\n",       room_type_str(r->type));
    printf("  Capacidade  : %d pessoa(s)\n", r->capacity);
    printf("  Diaria      : R$ %.2f\n",  r->daily_rate);
    printf("  Status      : %s\n",       room_status_str(r->status));
    printf("  Limpeza     : %s\n",       room_clean_str(r->clean_status));
    printf("  Comodidades : %s\n",       feat);
    printf("  Manutencoes : %d registro(s)\n", r->maintenance_count);
    print_separator('-', 60);
}

void room_list(int status_filter) {
    int found = 0;
    clear_screen();

    if (status_filter < 0) {
        print_header("LISTA DE QUARTOS");
    } else {
        char title[60];
        snprintf(title, sizeof(title), "QUARTOS — %s",
                 room_status_str((RoomStatus)status_filter));
        print_header(title);
    }

    print_table_header();
    for (int i = 0; i < g_room_count; i++) {
        if (status_filter < 0
            || g_rooms[i].status == (RoomStatus)status_filter)
        {
            room_print_brief(&g_rooms[i]);
            found++;
        }
    }
    print_separator('-', 74);
    printf("  Total: %d quarto(s)\n", found);
    pause_screen();
}

void room_occupation_map(void) {
    int floors[50], nfloors = 0;
    clear_screen();
    print_header("MAPA DE OCUPACAO");

    /* Coleta andares únicos */
    for (int i = 0; i < g_room_count; i++) {
        int f = g_rooms[i].floor, dup = 0;
        for (int j = 0; j < nfloors; j++) {
            if (floors[j] == f) { dup = 1; break; }
        }
        if (!dup && nfloors < 50) floors[nfloors++] = f;
    }

    /* Ordena decrescente (andar maior no topo) */
    for (int i = 0; i < nfloors - 1; i++) {
        for (int j = i + 1; j < nfloors; j++) {
            if (floors[j] > floors[i]) {
                int tmp = floors[i]; floors[i] = floors[j]; floors[j] = tmp;
            }
        }
    }

    printf("  L=Livre  O=Ocupado  R=Reservado  M=Manutencao  B=Bloqueado\n\n");

    for (int fi = 0; fi < nfloors; fi++) {
        printf("  Andar %2d: ", floors[fi]);
        for (int i = 0; i < g_room_count; i++) {
            if (g_rooms[i].floor != floors[fi]) continue;
            char sym = 'L';
            switch (g_rooms[i].status) {
                case ROOM_OCUPADO:    sym = 'O'; break;
                case ROOM_RESERVADO:  sym = 'R'; break;
                case ROOM_MANUTENCAO: sym = 'M'; break;
                case ROOM_BLOQUEADO:  sym = 'B'; break;
                default: break;
            }
            printf("[%c:%s] ", sym, g_rooms[i].number);
        }
        printf("\n");
    }

    if (nfloors == 0) printf("  Nenhum quarto cadastrado.\n");
    printf("\n");
    pause_screen();
}

/* =========================================================
 *  CRUD e operações
 * ========================================================= */
void room_add(void) {
    if (!auth_require(ACCESS_GERENTE)) return;

    clear_screen();
    print_header("CADASTRAR QUARTO");

    if (g_room_count >= MAX_ROOMS) {
        printf("  [!] Limite maximo de quartos atingido (%d).\n", MAX_ROOMS);
        pause_screen();
        return;
    }

    Room r;
    memset(&r, 0, sizeof(r));

    input_string("  Numero do quarto (ex: 101, 304A): ", r.number, sizeof(r.number));
    str_upper(r.number);

    if (room_find_by_number(r.number) != NULL) {
        printf("  [!] Quarto '%s' ja existe.\n", r.number);
        pause_screen();
        return;
    }

    r.floor      = input_int("  Andar", 0, 99);
    printf("  Tipo: [0] Solteiro  [1] Duplo  [2] Suite  [3] Presidencial\n");
    r.type       = (RoomType)input_int("  Tipo", 0, 3);
    r.capacity   = input_int("  Capacidade (pessoas)", 1, 10);
    r.daily_rate = input_double("  Diaria (R$)", 1.0, 99999.99);

    printf("  Comodidades:\n");
    r.features = 0;
    if (input_confirm("    Ar-condicionado")) r.features |= FEAT_AC;
    if (input_confirm("    Frigobar"))        r.features |= FEAT_FRIGOBAR;
    if (input_confirm("    Banheira"))        r.features |= FEAT_BANHEIRA;
    if (input_confirm("    Varanda"))         r.features |= FEAT_VARANDA;
    if (input_confirm("    Acessivel"))       r.features |= FEAT_ACESSIVEL;

    r.id           = storage_next_room_id();
    r.status       = ROOM_LIVRE;
    r.clean_status = CLEAN_LIMPO;

    g_rooms[g_room_count++] = r;
    storage_save_rooms();

    char log[80];
    snprintf(log, sizeof(log), "QUARTO ADD id=%d num=%s", r.id, r.number);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Quarto %s cadastrado (ID: %d).\n", r.number, r.id);
    pause_screen();
}

void room_edit(void) {
    if (!auth_require(ACCESS_GERENTE)) return;

    clear_screen();
    print_header("EDITAR QUARTO");

    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);

    Room *r = room_find_by_number(num);
    if (r == NULL) {
        printf("  [!] Quarto '%s' nao encontrado.\n", num);
        pause_screen();
        return;
    }

    room_print_full(r);
    printf("  O que deseja alterar?\n");
    printf("  [1] Tipo   [2] Capacidade   [3] Diaria   [4] Comodidades   [0] Cancelar\n");
    int opt = input_int("  Opcao", 0, 4);

    switch (opt) {
        case 1:
            printf("  Tipo atual: %s\n", room_type_str(r->type));
            printf("  [0] Solteiro  [1] Duplo  [2] Suite  [3] Presidencial\n");
            r->type = (RoomType)input_int("  Novo tipo", 0, 3);
            break;
        case 2:
            printf("  Capacidade atual: %d\n", r->capacity);
            r->capacity = input_int("  Nova capacidade", 1, 10);
            break;
        case 3:
            printf("  Diaria atual: R$ %.2f\n", r->daily_rate);
            r->daily_rate = input_double("  Nova diaria (R$)", 1.0, 99999.99);
            break;
        case 4: {
            char fb[128];
            room_features_str(r->features, fb, sizeof(fb));
            printf("  Comodidades atuais: %s\n  Reconfigurar:\n", fb);
            r->features = 0;
            if (input_confirm("    Ar-condicionado")) r->features |= FEAT_AC;
            if (input_confirm("    Frigobar"))        r->features |= FEAT_FRIGOBAR;
            if (input_confirm("    Banheira"))        r->features |= FEAT_BANHEIRA;
            if (input_confirm("    Varanda"))         r->features |= FEAT_VARANDA;
            if (input_confirm("    Acessivel"))       r->features |= FEAT_ACESSIVEL;
            break;
        }
        case 0:
            return;
    }

    storage_save_rooms();
    char log[80];
    snprintf(log, sizeof(log), "QUARTO EDIT id=%d campo=%d", r->id, opt);
    storage_audit_log(g_session.staff.id, log);
    printf("\n  [OK] Quarto %s atualizado.\n", r->number);
    pause_screen();
}

void room_change_status(void) {
    clear_screen();
    print_header("ALTERAR STATUS DO QUARTO");

    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);

    Room *r = room_find_by_number(num);
    if (r == NULL) {
        printf("  [!] Quarto '%s' nao encontrado.\n", num);
        pause_screen();
        return;
    }

    printf("\n  Status atual: %s\n\n", room_status_str(r->status));
    printf("  [0] Livre   [1] Ocupado   [2] Reservado   [3] Manutencao   [4] Bloqueado\n");
    RoomStatus new_status = (RoomStatus)input_int("  Novo status", 0, 4);

    /* Manutencao e Bloqueado exigem nível supervisor */
    if ((new_status == ROOM_MANUTENCAO || new_status == ROOM_BLOQUEADO)
        && !auth_require(ACCESS_SUPERVISOR))
    {
        return;
    }

    r->status = new_status;
    storage_save_rooms();

    char log[80];
    snprintf(log, sizeof(log), "QUARTO STATUS id=%d -> %s",
             r->id, room_status_str(new_status));
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Quarto %s: status alterado para %s.\n",
           r->number, room_status_str(new_status));
    pause_screen();
}

void room_add_maintenance(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("REGISTRAR MANUTENCAO");

    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);

    Room *r = room_find_by_number(num);
    if (r == NULL) {
        printf("  [!] Quarto '%s' nao encontrado.\n", num);
        pause_screen();
        return;
    }

    if (r->maintenance_count >= MAX_MAINT_PER_ROOM) {
        printf("  [!] Historico de manutencao cheio para este quarto.\n");
        pause_screen();
        return;
    }

    Maintenance *m = &r->maintenance_history[r->maintenance_count];
    memset(m, 0, sizeof(Maintenance));
    m->id = r->maintenance_count + 1;

    input_string("  Descricao        : ", m->description, sizeof(m->description));
    input_string("  Tecnico responsavel: ", m->technician, sizeof(m->technician));
    m->cost = input_double("  Custo (R$)", 0.0, 999999.99);
    m->date = time(NULL);

    r->maintenance_count++;
    r->status = ROOM_MANUTENCAO;
    storage_save_rooms();

    char log[80];
    snprintf(log, sizeof(log), "MANUTENCAO ADD quarto=%s custo=%.2f",
             r->number, m->cost);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Manutencao registrada. Quarto marcado como MANUTENCAO.\n");
    pause_screen();
}

void room_maintenance_history(void) {
    clear_screen();
    print_header("HISTORICO DE MANUTENCAO");

    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);

    Room *r = room_find_by_number(num);
    if (r == NULL) {
        printf("  [!] Quarto '%s' nao encontrado.\n", num);
        pause_screen();
        return;
    }

    printf("\n  Quarto %s  —  %d registro(s)\n\n",
           r->number, r->maintenance_count);

    if (r->maintenance_count == 0) {
        printf("  Nenhuma manutencao registrada.\n");
    } else {
        char dt[20];
        for (int i = 0; i < r->maintenance_count; i++) {
            Maintenance *m = &r->maintenance_history[i];
            datetime_to_string(m->date, dt);
            printf("  [%d] %s\n",       i + 1, dt);
            printf("       Descricao : %s\n", m->description);
            printf("       Tecnico   : %s\n", m->technician);
            printf("       Custo     : R$ %.2f\n", m->cost);
            print_separator('-', 50);
        }
    }
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static void room_list_all_wrap(void)   { room_list(-1); }
static void room_list_livre_wrap(void) { room_list(ROOM_LIVRE); }
static void room_list_ocup_wrap(void)  { room_list(ROOM_OCUPADO); }

static const MenuItem room_items[] = {
    {1, "Listar todos os quartos",       ACCESS_BASICO,    room_list_all_wrap},
    {2, "Listar quartos livres",          ACCESS_BASICO,    room_list_livre_wrap},
    {3, "Listar quartos ocupados",        ACCESS_BASICO,    room_list_ocup_wrap},
    {4, "Mapa de ocupacao",               ACCESS_BASICO,    room_occupation_map},
    {5, "Cadastrar quarto",               ACCESS_GERENTE,   room_add},
    {6, "Editar quarto",                  ACCESS_GERENTE,   room_edit},
    {7, "Alterar status",                 ACCESS_OPERADOR,  room_change_status},
    {8, "Registrar manutencao",           ACCESS_OPERADOR,  room_add_maintenance},
    {9, "Historico de manutencao",        ACCESS_BASICO,    room_maintenance_history},
    {0, "Voltar",                         0,                NULL}
};

void room_menu(void) {
    menu_run("MODULO: QUARTOS",
             room_items,
             (int)(sizeof(room_items) / sizeof(room_items[0])));
}
