#include "reservations.h"
#include "rooms.h"
#include "guests.h"
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
const char *res_status_str(ReservationStatus s) {
    switch (s) {
        case RES_PENDENTE:       return "PENDENTE";
        case RES_CONFIRMADA:     return "CONFIRMADA";
        case RES_CHECKIN_FEITO:  return "CHECKIN FEITO";
        case RES_CHECKOUT_FEITO: return "CHECKOUT FEITO";
        case RES_CANCELADA:      return "CANCELADA";
        case RES_NO_SHOW:        return "NO-SHOW";
        default:                 return "?";
    }
}

/* =========================================================
 *  Busca
 * ========================================================= */
Reservation *res_find_by_id(int id) {
    for (int i = 0; i < g_reservation_count; i++) {
        if (g_reservations[i].id == id) return &g_reservations[i];
    }
    return NULL;
}

Reservation *res_find_by_code(const char *code) {
    for (int i = 0; i < g_reservation_count; i++) {
        if (strcmp(g_reservations[i].code, code) == 0) return &g_reservations[i];
    }
    return NULL;
}

/* =========================================================
 *  Cálculo do valor base (diárias)
 * ========================================================= */
double res_calc_base_value(const Reservation *r) {
    int nights = date_diff_days(r->check_in_date, r->check_out_date);
    if (nights <= 0) nights = 1;

    double total = 0.0;
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        if (rm) total += rm->daily_rate * nights;
    }
    return total;
}

/* =========================================================
 *  Exibição
 * ========================================================= */
static void print_res_table_header(void) {
    printf("  %-4s %-10s %-25s %-12s %-13s %-13s %s\n",
           "ID", "Codigo", "Hospede", "Status",
           "Entrada", "Saida", "Total");
    print_separator('-', 92);
}

void res_print_brief(const Reservation *r) {
    Guest *g  = guest_find_by_id(r->guest_id);
    char  ci[14], co[14];
    date_to_string(r->check_in_date,  ci);
    date_to_string(r->check_out_date, co);

    printf("  %-4d %-10s %-25s %-13s %-13s %-13s R$%.2f\n",
           r->id, r->code,
           g ? g->full_name : "(hospede?)",
           res_status_str(r->status),
           ci, co,
           r->total_value);
}

void res_print_full(const Reservation *r) {
    Guest *g = guest_find_by_id(r->guest_id);
    char ci[14], co[14], cr[20];
    date_to_string(r->check_in_date,  ci);
    date_to_string(r->check_out_date, co);
    datetime_to_string(r->created_at, cr);

    int nights = date_diff_days(r->check_in_date, r->check_out_date);
    if (nights <= 0) nights = 1;

    print_separator('-', 60);
    printf("  Reserva #%d  [%s]\n", r->id, r->code);
    print_separator('-', 60);
    printf("  Status      : %s\n",       res_status_str(r->status));
    printf("  Hospede     : %s (ID: %d)\n",
           g ? g->full_name : "?", r->guest_id);
    printf("  Check-in    : %s\n",        ci);
    printf("  Check-out   : %s  (%d noite(s))\n", co, nights);
    printf("  Adultos/Cri.: %d / %d\n",   r->adults, r->children);
    printf("  Quartos     : ");
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        printf("%s ", rm ? rm->number : "?");
    }
    printf("\n");
    printf("  Diarias     : R$ %.2f\n",   r->base_value);
    printf("  Servicos    : R$ %.2f\n",   r->services_total);
    printf("  Desconto    : R$ %.2f\n",   r->discount);
    printf("  TOTAL       : R$ %.2f\n",   r->total_value);
    if (r->notes[0])
        printf("  Observacao  : %s\n",    r->notes);
    printf("  Criado em   : %s\n",        cr);
    print_separator('-', 60);
}

/* =========================================================
 *  Listagem / busca
 * ========================================================= */
void res_list(int status_filter) {
    clear_screen();
    char title[60];
    if (status_filter < 0)
        snprintf(title, sizeof(title), "LISTA DE RESERVAS");
    else
        snprintf(title, sizeof(title), "RESERVAS — %s",
                 res_status_str((ReservationStatus)status_filter));
    print_header(title);

    int found = 0;
    print_res_table_header();
    for (int i = 0; i < g_reservation_count; i++) {
        if (status_filter < 0
            || g_reservations[i].status == (ReservationStatus)status_filter)
        {
            res_print_brief(&g_reservations[i]);
            found++;
        }
    }
    print_separator('-', 92);
    printf("  Total: %d reserva(s)\n", found);
    pause_screen();
}

void res_list_by_guest(int guest_id) {
    int found = 0;
    print_res_table_header();
    for (int i = 0; i < g_reservation_count; i++) {
        if (g_reservations[i].guest_id == guest_id) {
            res_print_brief(&g_reservations[i]);
            found++;
        }
    }
    print_separator('-', 92);
    printf("  Total: %d reserva(s) do hospede.\n", found);
}

void res_search(void) {
    clear_screen();
    print_header("BUSCAR RESERVA");
    printf("  [1] Por codigo   [2] Por ID   [3] Por hospede (nome/doc)   [0] Cancelar\n");
    int opt = input_int("  Opcao", 0, 3);
    if (opt == 0) return;

    Reservation *r = NULL;

    if (opt == 1) {
        char code[12];
        input_string("  Codigo (ex: RES-A3F7K2): ", code, sizeof(code));
        str_upper(code);
        r = res_find_by_code(code);
    } else if (opt == 2) {
        int id = input_int("  ID da reserva", 1, 999999);
        r = res_find_by_id(id);
    } else {
        char term[100];
        input_string("  Nome ou documento: ", term, sizeof(term));
        int found = 0;
        printf("\n");
        print_res_table_header();
        for (int i = 0; i < g_reservation_count; i++) {
            Guest *g = guest_find_by_id(g_reservations[i].guest_id);
            if (!g) continue;
            if (str_contains(g->full_name, term)
                || str_contains(g->document, term))
            {
                res_print_brief(&g_reservations[i]);
                found++;
            }
        }
        print_separator('-', 92);
        printf("  %d resultado(s).\n", found);
        if (found > 0 && input_confirm("\n  Ver ficha completa de uma reserva")) {
            int id = input_int("  ID da reserva", 1, 999999);
            r = res_find_by_id(id);
        }
        if (!r) { pause_screen(); return; }
    }

    if (!r) {
        printf("\n  [!] Reserva nao encontrada.\n");
    } else {
        res_print_full(r);
    }
    pause_screen();
}

/* =========================================================
 *  Helpers internos
 * ========================================================= */

/* Verifica conflito de datas para um quarto */
static int room_has_conflict(int room_id, time_t ci, time_t co, int exclude_res_id) {
    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->id == exclude_res_id) continue;
        if (r->status == RES_CANCELADA || r->status == RES_CHECKOUT_FEITO
            || r->status == RES_NO_SHOW) continue;

        for (int j = 0; j < r->room_count; j++) {
            if (r->room_ids[j] != room_id) continue;
            /* Conflito se os intervalos se sobrepõem */
            if (ci < r->check_out_date && co > r->check_in_date) return 1;
        }
    }
    return 0;
}

/* =========================================================
 *  CRUD e operações
 * ========================================================= */
void res_add(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("NOVA RESERVA");

    if (g_reservation_count >= MAX_RESERVATIONS) {
        printf("  [!] Limite maximo de reservas atingido.\n");
        pause_screen();
        return;
    }

    Reservation r;
    memset(&r, 0, sizeof(r));

    /* ── Hóspede ── */
    printf("  Buscar hospede por: [1] ID   [2] Documento\n");
    int bo = input_int("  Opcao", 1, 2);
    Guest *g = NULL;
    if (bo == 1) {
        int gid = input_int("  ID do hospede", 1, 999999);
        g = guest_find_by_id(gid);
    } else {
        char doc[20];
        input_string("  Documento: ", doc, sizeof(doc));
        g = guest_find_by_document(doc);
    }

    if (!g) {
        printf("  [!] Hospede nao encontrado.\n");
        pause_screen();
        return;
    }
    if (g->blacklisted) {
        printf("  [!] Hospede esta na BLACKLIST: %s\n", g->blacklist_reason);
        if (!input_confirm("  Deseja prosseguir mesmo assim")) {
            pause_screen();
            return;
        }
    }
    r.guest_id = g->id;
    printf("  Hospede: %s\n\n", g->full_name);

    /* ── Datas ── */
    char ci_str[12], co_str[12];
    do {
        input_string("  Data de check-in  (DD/MM/AAAA): ", ci_str, sizeof(ci_str));
        if (!date_is_valid(ci_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(ci_str));

    do {
        input_string("  Data de check-out (DD/MM/AAAA): ", co_str, sizeof(co_str));
        if (!date_is_valid(co_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(co_str));

    r.check_in_date  = date_from_string(ci_str);
    r.check_out_date = date_from_string(co_str);

    if (r.check_out_date <= r.check_in_date) {
        printf("  [!] Data de check-out deve ser posterior ao check-in.\n");
        pause_screen();
        return;
    }

    int nights = date_diff_days(r.check_in_date, r.check_out_date);

    /* ── Quartos ── */
    r.room_count = input_int("  Numero de quartos na reserva", 1, MAX_ROOMS_PER_RES);
    for (int i = 0; i < r.room_count; i++) {
        char num[8];
        printf("  Quarto %d — ", i + 1);
        input_string("numero: ", num, sizeof(num));
        str_upper(num);

        Room *rm = room_find_by_number(num);
        if (!rm) {
            printf("  [!] Quarto '%s' nao existe.\n", num);
            i--; continue;
        }
        if (rm->status == ROOM_MANUTENCAO || rm->status == ROOM_BLOQUEADO) {
            printf("  [!] Quarto %s esta %s e nao pode ser reservado.\n",
                   num, room_status_str(rm->status));
            i--; continue;
        }
        if (room_has_conflict(rm->id, r.check_in_date, r.check_out_date, -1)) {
            printf("  [!] Quarto %s ja tem reserva neste periodo.\n", num);
            i--; continue;
        }
        if (rm->capacity < r.adults + r.children && r.adults > 0) {
            printf("  [!] Aviso: capacidade do quarto (%d) pode ser insuficiente.\n",
                   rm->capacity);
        }
        r.room_ids[i] = rm->id;
    }

    r.adults   = input_int("  Num. de adultos", 1, 20);
    r.children = input_int("  Num. de criancas", 0, 20);

    input_string("  Observacoes (Enter para pular): ", r.notes, sizeof(r.notes));

    /* ── Valores ── */
    r.base_value   = res_calc_base_value(&r);
    r.total_value  = r.base_value;

    /* ── Geração automática ── */
    r.id          = storage_next_reservation_id();
    r.status      = RES_CONFIRMADA;
    r.created_by  = g_session.staff.id;
    r.created_at  = time(NULL);
    generate_confirmation_code(r.code);

    /* Garante código único */
    while (res_find_by_code(r.code) != NULL) {
        generate_confirmation_code(r.code);
    }

    /* Marca quartos como RESERVADO */
    for (int i = 0; i < r.room_count; i++) {
        Room *rm = room_find_by_id(r.room_ids[i]);
        if (rm && rm->status == ROOM_LIVRE) rm->status = ROOM_RESERVADO;
    }
    storage_save_rooms();

    g_reservations[g_reservation_count++] = r;
    storage_save_reservations();

    char log[80];
    snprintf(log, sizeof(log), "RESERVA ADD id=%d code=%s guest=%d nights=%d",
             r.id, r.code, r.guest_id, nights);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Reserva criada!\n");
    printf("  Codigo de confirmacao: %s\n", r.code);
    printf("  Periodo: %s a %s (%d noite(s))\n", ci_str, co_str, nights);
    printf("  Valor base das diarias: R$ %.2f\n", r.base_value);
    pause_screen();
}

void res_cancel(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("CANCELAR RESERVA");

    int id = input_int("  ID da reserva", 1, 999999);
    Reservation *r = res_find_by_id(id);
    if (!r) {
        printf("  [!] Reserva ID %d nao encontrada.\n", id);
        pause_screen();
        return;
    }
    if (r->status == RES_CANCELADA) {
        printf("  [!] Reserva ja esta cancelada.\n");
        pause_screen();
        return;
    }
    if (r->status == RES_CHECKOUT_FEITO) {
        printf("  [!] Nao e possivel cancelar uma reserva com checkout realizado.\n");
        pause_screen();
        return;
    }
    if (r->status == RES_CHECKIN_FEITO) {
        if (!auth_require(ACCESS_GERENTE)) return;
    }

    res_print_brief(r);
    if (!input_confirm("\n  Confirma cancelamento")) return;

    /* Política de cancelamento */
    int days_to_ci = date_diff_days(date_today(), r->check_in_date);
    double penalty = 0.0;
    if (days_to_ci >= g_config.cancel_free_days) {
        printf("  Cancelamento sem custo (dentro do prazo).\n");
    } else if (days_to_ci > 0) {
        penalty = r->base_value * g_config.cancel_partial_pct;
        printf("  Taxa de cancelamento (%.0f%%): R$ %.2f\n",
               g_config.cancel_partial_pct * 100.0, penalty);
    } else {
        penalty = r->base_value;
        printf("  Sem reembolso (no prazo de tolerancia ou apos check-in).\n");
    }

    r->status   = RES_CANCELADA;
    r->discount = r->base_value - penalty;  /* indica quanto foi devolvido */

    /* Libera quartos */
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        if (rm && rm->status == ROOM_RESERVADO) rm->status = ROOM_LIVRE;
    }
    storage_save_rooms();
    storage_save_reservations();

    char log[80];
    snprintf(log, sizeof(log), "RESERVA CANCEL id=%d penalty=%.2f", r->id, penalty);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Reserva %s cancelada.\n", r->code);
    pause_screen();
}

void res_mark_noshow(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("MARCAR NO-SHOW");

    int id = input_int("  ID da reserva", 1, 999999);
    Reservation *r = res_find_by_id(id);
    if (!r) {
        printf("  [!] Reserva nao encontrada.\n");
        pause_screen();
        return;
    }
    if (r->status != RES_CONFIRMADA && r->status != RES_PENDENTE) {
        printf("  [!] Reserva com status '%s' nao pode ser marcada como no-show.\n",
               res_status_str(r->status));
        pause_screen();
        return;
    }

    res_print_brief(r);
    if (!input_confirm("\n  Confirma no-show")) return;

    r->status = RES_NO_SHOW;
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        if (rm && rm->status == ROOM_RESERVADO) rm->status = ROOM_LIVRE;
    }
    storage_save_rooms();
    storage_save_reservations();

    char log[80];
    snprintf(log, sizeof(log), "RESERVA NOSHOW id=%d", r->id);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Reserva marcada como NO-SHOW.\n");
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static void res_list_all_wrap(void)  { res_list(-1); }
static void res_list_conf_wrap(void) { res_list(RES_CONFIRMADA); }
static void res_list_ci_wrap(void)   { res_list(RES_CHECKIN_FEITO); }

static const MenuItem res_items[] = {
    {1, "Listar todas as reservas",  ACCESS_BASICO,    res_list_all_wrap},
    {2, "Listar confirmadas",        ACCESS_BASICO,    res_list_conf_wrap},
    {3, "Listar em hospedagem",      ACCESS_BASICO,    res_list_ci_wrap},
    {4, "Buscar reserva",            ACCESS_BASICO,    res_search},
    {5, "Nova reserva",              ACCESS_OPERADOR,  res_add},
    {6, "Cancelar reserva",          ACCESS_OPERADOR,  res_cancel},
    {7, "Marcar no-show",            ACCESS_SUPERVISOR,res_mark_noshow},
    {0, "Voltar",                    0,                NULL}
};

void res_menu(void) {
    menu_run("MODULO: RESERVAS",
             res_items,
             (int)(sizeof(res_items) / sizeof(res_items[0])));
}
