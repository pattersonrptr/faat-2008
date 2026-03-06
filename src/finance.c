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
 *  Menu do módulo
 * ========================================================= */
static const MenuItem finance_items[] = {
    {1, "Faturar reserva",       ACCESS_OPERADOR,   finance_invoice},
    {2, "Caixa do dia",          ACCESS_SUPERVISOR, finance_daily_report},
    {3, "Receita por periodo",   ACCESS_SUPERVISOR, finance_period_report},
    {4, "Contas a receber",      ACCESS_SUPERVISOR, finance_receivables},
    {0, "Voltar",                0,                 NULL}
};

void finance_menu(void) {
    menu_run("MODULO: FINANCEIRO",
             finance_items,
             (int)(sizeof(finance_items) / sizeof(finance_items[0])));
}
