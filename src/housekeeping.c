#include "housekeeping.h"
#include "rooms.h"
#include "reservations.h"
#include "storage.h"
#include "staff.h"
#include "auth.h"
#include "utils.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* =========================================================
 *  Prioridade de limpeza
 *    1 = checkout feito hoje (máxima urgência)
 *    2 = ocupado (precisa de limpeza periódica)
 *    3 = livre/reservado
 * ========================================================= */
static int clean_priority(const Room *r) {
    if (r->status == ROOM_OCUPADO)  return 2;
    if (r->status == ROOM_LIVRE)    return 3;
    return 3;
}

/* =========================================================
 *  Lista diária de quartos para limpeza
 * ========================================================= */
void hk_daily_list(void) {
    clear_screen();
    print_header("LISTA DE LIMPEZA DO DIA");

    /* Coloca índices em array e ordena por prioridade */
    int idx[MAX_ROOMS], n = 0;
    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].status == ROOM_MANUTENCAO
            || g_rooms[i].status == ROOM_BLOQUEADO) continue;
        if (g_rooms[i].clean_status == CLEAN_INSPECIONADO) continue;
        idx[n++] = i;
    }

    /* Bubble sort simples por prioridade (listas pequenas) */
    for (int i = 0; i < n - 1; i++) {
        for (int j = i + 1; j < n; j++) {
            if (clean_priority(&g_rooms[idx[j]]) < clean_priority(&g_rooms[idx[i]])) {
                int tmp = idx[i]; idx[i] = idx[j]; idx[j] = tmp;
            }
        }
    }

    printf("  %-6s %-5s %-12s %-14s %-10s %s\n",
           "Num.", "And.", "Status", "Limpeza", "Camareiro", "Prioridade");
    print_separator('-', 60);

    for (int k = 0; k < n; k++) {
        Room *r = &g_rooms[idx[k]];
        int   pri = clean_priority(r);
        Staff *s  = (r->assigned_staff_id > 0)
                    ? staff_find_by_id(r->assigned_staff_id) : NULL;

        printf("  %-6s %-5d %-12s %-14s %-10s %s\n",
               r->number, r->floor,
               room_status_str(r->status),
               room_clean_str(r->clean_status),
               s ? s->full_name : "-",
               pri == 1 ? "!! URGENTE" : pri == 2 ? "! NORMAL" : "  BAIXA");
    }
    print_separator('-', 60);
    printf("  %d quarto(s) a limpar.\n", n);
    pause_screen();
}

/* =========================================================
 *  Atualizar status de limpeza
 * ========================================================= */
void hk_update_clean_status(void) {
    if (!auth_require(ACCESS_BASICO)) return;

    clear_screen();
    print_header("ATUALIZAR LIMPEZA");

    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);

    Room *r = room_find_by_number(num);
    if (!r) {
        printf("  [!] Quarto '%s' nao encontrado.\n", num);
        pause_screen();
        return;
    }

    printf("  Status atual: %s\n\n", room_clean_str(r->clean_status));
    printf("  [0] Sujo   [1] Em limpeza   [2] Limpo   [3] Inspecionado\n");
    r->clean_status = (CleanStatus)input_int("  Novo status", 0, 3);

    storage_save_rooms();
    char log[60];
    snprintf(log, sizeof(log), "HK CLEAN %s -> %s",
             num, room_clean_str(r->clean_status));
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Quarto %s: limpeza = %s\n", num, room_clean_str(r->clean_status));
    pause_screen();
}

/* =========================================================
 *  Atribuir camareiro a quarto
 * ========================================================= */
void hk_assign_room(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("ATRIBUIR CAMAREIRO");

    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);

    Room *r = room_find_by_number(num);
    if (!r) {
        printf("  [!] Quarto '%s' nao encontrado.\n", num);
        pause_screen();
        return;
    }

    Staff *cur = (r->assigned_staff_id > 0)
                 ? staff_find_by_id(r->assigned_staff_id) : NULL;
    printf("  Camareiro atual: %s\n\n",
           cur ? cur->full_name : "nenhum");

    int sid = input_int("  ID do funcionario (0 = remover atribuicao)", 0, 999999);
    if (sid > 0) {
        Staff *s = staff_find_by_id(sid);
        if (!s || !s->active) {
            printf("  [!] Funcionario nao encontrado ou inativo.\n");
            pause_screen();
            return;
        }
        r->assigned_staff_id = sid;
        printf("\n  [OK] Quarto %s atribuido a %s.\n", num, s->full_name);
    } else {
        r->assigned_staff_id = 0;
        printf("\n  [OK] Atribuicao removida do quarto %s.\n", num);
    }

    storage_save_rooms();
    pause_screen();
}

/* =========================================================
 *  Objetos esquecidos
 * ========================================================= */
void hk_lost_item_add(void) {
    if (!auth_require(ACCESS_BASICO)) return;

    clear_screen();
    print_header("REGISTRAR OBJETO ESQUECIDO");

    if (g_lost_item_count >= MAX_LOST_ITEMS) {
        printf("  [!] Limite de objetos esquecidos atingido.\n");
        pause_screen();
        return;
    }

    LostItem li;
    memset(&li, 0, sizeof(li));

    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);
    Room *r = room_find_by_number(num);
    if (!r) {
        printf("  [!] Quarto '%s' nao encontrado.\n", num);
        pause_screen();
        return;
    }

    li.room_id    = r->id;
    li.found_date = time(NULL);
    li.returned   = 0;

    input_string("  Descricao do objeto  : ", li.item_description, sizeof(li.item_description));
    input_string("  Observacoes/localizacao: ", li.finder_notes,   sizeof(li.finder_notes));
    li.id = storage_next_lost_item_id();

    g_lost_items[g_lost_item_count++] = li;
    storage_save_lost_items();

    char log[80];
    snprintf(log, sizeof(log), "LOST_ITEM ADD id=%d room=%s", li.id, num);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Objeto registrado (ID: %d).\n", li.id);
    pause_screen();
}

void hk_lost_item_list(void) {
    clear_screen();
    print_header("OBJETOS ESQUECIDOS");

    printf("  [1] Todos   [2] Apenas guardados (nao devolvidos)   [0] Cancelar\n");
    int opt = input_int("  Filtro", 0, 2);
    if (opt == 0) return;

    printf("\n  %-4s %-6s %-30s %-8s %s\n",
           "ID", "Quarto", "Objeto", "Data", "Status");
    print_separator('-', 65);

    int found = 0;
    time_t now = time(NULL);
    for (int i = 0; i < g_lost_item_count; i++) {
        LostItem *li = &g_lost_items[i];
        if (opt == 2 && li->returned != 0) continue;

        Room *r = room_find_by_id(li->room_id);
        char dt[14];
        date_to_string(li->found_date > 0 ? li->found_date : now, dt);
        const char *status_label = (li->returned == 0) ? "Guardado"
                                 : (li->returned == 1) ? "Devolvido"
                                 : "Descartado";

        printf("  %-4d %-6s %-30s %-8s %s\n",
               li->id,
               r ? r->number : "?",
               li->item_description,
               dt,
               status_label);
        found++;
    }
    print_separator('-', 65);
    printf("  %d item(ns) encontrado(s).\n", found);
    pause_screen();
}

void hk_lost_item_resolve(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("RESOLVER OBJETO ESQUECIDO");

    int id = input_int("  ID do objeto", 1, 999999);
    LostItem *li = NULL;
    for (int i = 0; i < g_lost_item_count; i++) {
        if (g_lost_items[i].id == id) { li = &g_lost_items[i]; break; }
    }
    if (!li) {
        printf("  [!] Objeto ID %d nao encontrado.\n", id);
        pause_screen();
        return;
    }

    printf("  Objeto  : %s\n", li->item_description);
    printf("  Status atual: %s\n\n",
           li->returned == 0 ? "Guardado" : li->returned == 1 ? "Devolvido" : "Descartado");

    printf("  [1] Marcar como devolvido   [2] Marcar como descartado   [0] Cancelar\n");
    int opt = input_int("  Opcao", 0, 2);
    if (opt == 0) return;

    li->returned = opt; /* 1=devolvido, 2=descartado */
    storage_save_lost_items();

    char log[60];
    snprintf(log, sizeof(log), "LOST_ITEM RESOLVE id=%d status=%d", li->id, li->returned);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Status atualizado.\n");
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static const MenuItem hk_items[] = {
    {1, "Lista de limpeza do dia",   ACCESS_BASICO,    hk_daily_list},
    {2, "Atualizar status limpeza",  ACCESS_BASICO,    hk_update_clean_status},
    {3, "Atribuir camareiro",        ACCESS_SUPERVISOR,hk_assign_room},
    {4, "Registrar objeto esquecido",ACCESS_BASICO,    hk_lost_item_add},
    {5, "Listar objetos esquecidos", ACCESS_BASICO,    hk_lost_item_list},
    {6, "Resolver objeto esquecido", ACCESS_OPERADOR,  hk_lost_item_resolve},
    {0, "Voltar",                    0,                NULL}
};

void hk_menu(void) {
    menu_run("MODULO: GOVERNANCA / HOUSEKEEPING",
             hk_items,
             (int)(sizeof(hk_items) / sizeof(hk_items[0])));
}
