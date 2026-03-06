#include "finance.h"
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
 *  Helpers
 * ========================================================= */
const char *pay_method_str(PaymentMethod m) {
    switch (m) {
        case PAY_DINHEIRO:      return "Dinheiro";
        case PAY_CREDITO:       return "Credito";
        case PAY_DEBITO:        return "Debito";
        case PAY_PIX:           return "PIX";
        case PAY_TRANSFERENCIA: return "Transferencia";
        case PAY_FATURADO:      return "Faturado";
        default:                return "?";
    }
}

void finance_calc_total(Reservation *r) {
    r->base_value = res_calc_base_value(r);

    r->services_total = 0.0;
    for (int i = 0; i < g_service_count; i++) {
        if (g_services[i].reservation_id == r->id
            && g_services[i].status != SVC_STATUS_CANCELADO)
        {
            r->services_total += g_services[i].unit_price
                                 * g_services[i].quantity;
        }
    }

    double subtotal   = r->base_value + r->services_total;
    double tax        = subtotal * g_config.service_fee_pct
                        + g_config.tourist_tax
                          * date_diff_days(r->check_in_date, r->check_out_date)
                          * r->room_count;
    r->total_value    = subtotal + tax - r->discount;
    if (r->total_value < 0.0) r->total_value = 0.0;
}

/* =========================================================
 *  Faturamento avulso (para reservas já finalizadas ou faturadas)
 * ========================================================= */
void finance_invoice(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("FATURAR RESERVA");

    int id = input_int("  ID da reserva", 1, 999999);
    Reservation *r = res_find_by_id(id);
    if (!r) {
        printf("  [!] Reserva nao encontrada.\n");
        pause_screen();
        return;
    }
    if (r->status != RES_CHECKOUT_FEITO && r->status != RES_CHECKIN_FEITO) {
        printf("  [!] So e possivel faturar reservas em hospedagem ou com checkout feito.\n");
        pause_screen();
        return;
    }

    finance_calc_total(r);

    int nights = date_diff_days(r->check_in_date, r->check_out_date);
    if (nights <= 0) nights = 1;
    double tax = (r->base_value + r->services_total)
                 * g_config.service_fee_pct
                 + g_config.tourist_tax * nights * r->room_count;

    Guest *g = guest_find_by_id(r->guest_id);

    print_separator('=', 60);
    printf("  FATURA — Reserva %s\n", r->code);
    print_separator('=', 60);
    printf("  Hospede   : %s (ID: %d)\n", g ? g->full_name : "?", r->guest_id);
    printf("  Dias      : %d  |  Quartos: %d\n", nights, r->room_count);
    print_separator('-', 60);
    printf("  Diarias           : R$ %8.2f\n", r->base_value);
    printf("  Servicos          : R$ %8.2f\n", r->services_total);
    printf("  Taxas             : R$ %8.2f\n", tax);
    if (r->discount > 0.0)
        printf("  Desconto          : R$ %8.2f\n", r->discount);
    print_separator('-', 60);
    printf("  TOTAL             : R$ %8.2f\n", r->total_value);
    print_separator('=', 60);

    printf("  Forma pagamento atual: %s\n\n", pay_method_str(r->payment_method));

    if (input_confirm("  Alterar forma de pagamento")) {
        printf("  [0]Dinheiro [1]Credito [2]Debito [3]PIX [4]Transferencia [5]Faturado\n");
        r->payment_method = (PaymentMethod)input_int("  Opcao", 0, 5);
        if (r->payment_method == PAY_CREDITO)
            r->payment_installments = input_int("  Parcelas", 1, 12);
    }

    if (input_confirm("  Aplicar desconto")) {
        double disc = input_double("  Valor do desconto (R$)", 0.0, r->total_value);
        r->discount    += disc;
        r->total_value -= disc;
        if (r->total_value < 0.0) r->total_value = 0.0;
    }

    storage_save_reservations();

    char log[80];
    snprintf(log, sizeof(log), "INVOICE id=%d total=%.2f method=%d",
             r->id, r->total_value, (int)r->payment_method);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Fatura atualizada. Total: R$ %.2f\n", r->total_value);
    pause_screen();
}

/* =========================================================
 *  Relatório de caixa do dia
 * ========================================================= */
void finance_daily_report(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("CAIXA DO DIA");

    time_t today_start = date_today();
    time_t today_end   = today_start + 86400;

    double total    = 0.0, by_method[6] = {0};
    int    count    = 0;

    printf("  %-10s %-14s %-25s %-14s %s\n",
           "Res. ID", "Codigo", "Hospede", "Pagamento", "Valor");
    print_separator('-', 75);

    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status != RES_CHECKOUT_FEITO) continue;
        if (r->actual_check_out < today_start
            || r->actual_check_out >= today_end) continue;

        Guest *g = guest_find_by_id(r->guest_id);
        printf("  %-10d %-14s %-25s %-14s R$ %.2f\n",
               r->id, r->code,
               g ? g->full_name : "?",
               pay_method_str(r->payment_method),
               r->total_value);
        total += r->total_value;
        if ((int)r->payment_method < 6)
            by_method[(int)r->payment_method] += r->total_value;
        count++;
    }

    print_separator('-', 75);
    printf("  Checkouts hoje: %d   |   Total: R$ %.2f\n\n", count, total);
    printf("  Breakdown por forma de pagamento:\n");
    for (int m = 0; m < 6; m++) {
        if (by_method[m] > 0.0)
            printf("    %-14s: R$ %.2f\n",
                   pay_method_str((PaymentMethod)m), by_method[m]);
    }
    pause_screen();
}

/* =========================================================
 *  Relatório por período
 * ========================================================= */
void finance_period_report(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("RECEITA POR PERIODO");

    char from_str[12], to_str[12];
    do {
        input_string("  Data inicial (DD/MM/AAAA): ", from_str, sizeof(from_str));
        if (!date_is_valid(from_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(from_str));
    do {
        input_string("  Data final   (DD/MM/AAAA): ", to_str, sizeof(to_str));
        if (!date_is_valid(to_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(to_str));

    time_t t_from = date_from_string(from_str);
    time_t t_to   = date_from_string(to_str) + 86400; /* inclui o dia final */

    double total_diarias  = 0.0;
    double total_servicos = 0.0;
    double total_descontos = 0.0;
    double grand_total    = 0.0;
    int    count          = 0;

    printf("\n  %-10s %-14s %-20s %-10s %-10s %s\n",
           "ID", "Codigo", "Hospede", "Diarias", "Servicos", "Total");
    print_separator('-', 80);

    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status != RES_CHECKOUT_FEITO) continue;
        if (r->actual_check_out < t_from || r->actual_check_out >= t_to) continue;

        Guest *g = guest_find_by_id(r->guest_id);
        printf("  %-10d %-14s %-20s R$%-8.2f R$%-8.2f R$%.2f\n",
               r->id, r->code,
               g ? g->full_name : "?",
               r->base_value, r->services_total, r->total_value);

        total_diarias  += r->base_value;
        total_servicos += r->services_total;
        total_descontos += r->discount;
        grand_total    += r->total_value;
        count++;
    }

    print_separator('-', 80);
    printf("  Reservas: %d\n", count);
    printf("  Diarias  : R$ %.2f\n",  total_diarias);
    printf("  Servicos : R$ %.2f\n",  total_servicos);
    printf("  Descontos: R$ %.2f\n",  total_descontos);
    printf("  TOTAL    : R$ %.2f\n",  grand_total);
    pause_screen();
}

/* =========================================================
 *  Contas a receber (reservas com PAY_FATURADO)
 * ========================================================= */
void finance_receivables(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("CONTAS A RECEBER");

    double total = 0.0;
    int    count = 0;

    printf("  %-10s %-14s %-25s %-14s %s\n",
           "ID", "Codigo", "Hospede", "Status", "Valor");
    print_separator('-', 75);

    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->payment_method != PAY_FATURADO) continue;
        if (r->status == RES_CANCELADA) continue;

        Guest *g = guest_find_by_id(r->guest_id);
        printf("  %-10d %-14s %-25s %-14s R$ %.2f\n",
               r->id, r->code,
               g ? g->full_name : "?",
               res_status_str(r->status),
               r->total_value);
        total += r->total_value;
        count++;
    }

    print_separator('-', 75);
    printf("  Total a receber: R$ %.2f  (%d conta(s))\n", total, count);
    pause_screen();
}

/* =========================================================
 *  Caixa por turno — helpers
 * ========================================================= */
static const char *shift_type_str(ShiftType t) {
    switch (t) {
        case SHIFT_MANHA: return "Manha";
        case SHIFT_TARDE: return "Tarde";
        case SHIFT_NOITE: return "Noite";
        default:          return "?";
    }
}

static CashShift *find_active_shift(void) {
    for (int i = 0; i < g_shift_count; i++) {
        if (g_shifts[i].active) return &g_shifts[i];
    }
    return NULL;
}

/* =========================================================
 *  Abrir turno
 * ========================================================= */
void finance_open_shift(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("ABRIR TURNO DE CAIXA");

    CashShift *active = find_active_shift();
    if (active) {
        char opened_str[20];
        datetime_to_string(active->opened_at, opened_str);
        printf("  [!] Ja existe um turno aberto:\n");
        printf("  Turno #%d — %s — %s — Operador: %s\n",
               active->id, shift_type_str(active->type),
               opened_str, active->operator_name);
        printf("  Feche o turno atual antes de abrir um novo.\n");
        pause_screen();
        return;
    }
    if (g_shift_count >= MAX_SHIFTS) {
        printf("  [!] Limite de turnos atingido.\n");
        pause_screen();
        return;
    }

    printf("  Tipo de turno:\n");
    printf("  [1] Manha   [2] Tarde   [3] Noite\n");
    int op = input_int("  Opcao", 1, 3);

    CashShift *s = &g_shifts[g_shift_count];
    memset(s, 0, sizeof(CashShift));
    s->id          = storage_next_shift_id();
    strncpy(s->operator_name, g_session.staff.full_name,
            sizeof(s->operator_name) - 1);
    s->operator_id = g_session.staff.id;
    s->type        = (ShiftType)(op - 1);
    s->opened_at   = time(NULL);
    s->active      = 1;
    g_shift_count++;

    storage_save_shifts();

    char log[80];
    snprintf(log, sizeof(log), "SHIFT OPEN id=%d type=%s",
             s->id, shift_type_str(s->type));
    storage_audit_log(g_session.staff.id, log);

    char opened_str[20];
    datetime_to_string(s->opened_at, opened_str);
    printf("\n  [OK] Turno #%d (%s) aberto em %s.\n",
           s->id, shift_type_str(s->type), opened_str);
    pause_screen();
}

/* =========================================================
 *  Fechar turno
 * ========================================================= */
void finance_close_shift(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("FECHAR TURNO DE CAIXA");

    CashShift *s = find_active_shift();
    if (!s) {
        printf("  Nenhum turno aberto no momento.\n");
        pause_screen();
        return;
    }

    char opened_str[20];
    datetime_to_string(s->opened_at, opened_str);
    printf("  Turno #%d — %s — Aberto em %s por %s\n\n",
           s->id, shift_type_str(s->type), opened_str, s->operator_name);

    if (!input_confirm("  Confirmar fechamento? (s/n): ")) {
        printf("  Operacao cancelada.\n");
        pause_screen();
        return;
    }

    s->closed_at = time(NULL);
    s->active    = 0;

    /* Totaliza checkouts ocorridos durante o turno */
    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status != RES_CHECKOUT_FEITO) continue;
        if (r->actual_check_out < s->opened_at ||
            r->actual_check_out >= s->closed_at) continue;

        switch (r->payment_method) {
            case PAY_DINHEIRO:      s->total_dinheiro      += r->total_value; break;
            case PAY_CREDITO:       s->total_credito       += r->total_value; break;
            case PAY_DEBITO:        s->total_debito        += r->total_value; break;
            case PAY_PIX:           s->total_pix           += r->total_value; break;
            case PAY_TRANSFERENCIA: s->total_transferencia += r->total_value; break;
            case PAY_FATURADO:      s->total_faturado      += r->total_value; break;
            default: break;
        }
        s->grand_total     += r->total_value;
        s->total_discounts += r->discount;
        s->checkout_count++;
    }

    storage_save_shifts();

    char log[80];
    snprintf(log, sizeof(log), "SHIFT CLOSE id=%d total=%.2f count=%d",
             s->id, s->grand_total, s->checkout_count);
    storage_audit_log(g_session.staff.id, log);

    /* Exibe extrato do turno fechado */
    printf("\n");
    print_separator('=', 55);
    printf("  EXTRATO DO TURNO #%d (%s)\n", s->id, shift_type_str(s->type));
    print_separator('-', 55);
    printf("  Operador  : %s\n", s->operator_name);
    char closed_str[20];
    datetime_to_string(s->closed_at, closed_str);
    printf("  Abertura  : %s\n", opened_str);
    printf("  Fechamento: %s\n", closed_str);
    print_separator('-', 55);
    printf("  Checkouts : %d\n", s->checkout_count);
    if (s->total_dinheiro      > 0.0)
        printf("  Dinheiro     : R$ %10.2f\n", s->total_dinheiro);
    if (s->total_credito       > 0.0)
        printf("  Credito      : R$ %10.2f\n", s->total_credito);
    if (s->total_debito        > 0.0)
        printf("  Debito       : R$ %10.2f\n", s->total_debito);
    if (s->total_pix           > 0.0)
        printf("  PIX          : R$ %10.2f\n", s->total_pix);
    if (s->total_transferencia > 0.0)
        printf("  Transferencia: R$ %10.2f\n", s->total_transferencia);
    if (s->total_faturado      > 0.0)
        printf("  Faturado     : R$ %10.2f\n", s->total_faturado);
    print_separator('-', 55);
    printf("  Descontos : R$ %10.2f\n", s->total_discounts);
    printf("  TOTAL     : R$ %10.2f\n", s->grand_total);
    print_separator('=', 55);
    pause_screen();
}

/* =========================================================
 *  Histórico de turnos
 * ========================================================= */
void finance_shift_report(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("HISTORICO DE TURNOS");

    if (g_shift_count == 0) {
        printf("  Nenhum turno registrado.\n");
        pause_screen();
        return;
    }

    int start = (g_shift_count > 20) ? g_shift_count - 20 : 0;
    printf("  %-5s %-9s %-20s %-17s %-8s %s\n",
           "#", "Tipo", "Operador", "Abertura", "Status", "Total");
    print_separator('-', 75);
    for (int i = start; i < g_shift_count; i++) {
        CashShift *cs = &g_shifts[i];
        char opened_str[20];
        datetime_to_string(cs->opened_at, opened_str);
        printf("  %-5d %-9s %-20s %-17s %-8s R$ %.2f\n",
               cs->id, shift_type_str(cs->type), cs->operator_name,
               opened_str,
               cs->active ? "ABERTO" : "FECHADO",
               cs->grand_total);
    }
    print_separator('-', 75);
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static const MenuItem finance_items[] = {
    {1, "Faturar reserva",       ACCESS_OPERADOR,   finance_invoice},
    {2, "Caixa do dia",          ACCESS_SUPERVISOR, finance_daily_report},
    {3, "Receita por periodo",   ACCESS_SUPERVISOR, finance_period_report},
    {4, "Contas a receber",      ACCESS_SUPERVISOR, finance_receivables},
    {5, "Abrir turno",           ACCESS_SUPERVISOR, finance_open_shift},
    {6, "Fechar turno",          ACCESS_SUPERVISOR, finance_close_shift},
    {7, "Historico de turnos",   ACCESS_SUPERVISOR, finance_shift_report},
    {0, "Voltar",                0,                 NULL}
};

void finance_menu(void) {
    menu_run("MODULO: FINANCEIRO",
             finance_items,
             (int)(sizeof(finance_items) / sizeof(finance_items[0])));
}
