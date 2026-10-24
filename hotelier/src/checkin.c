#include "checkin.h"
#include "reservations.h"
#include "rooms.h"
#include "guests.h"
#include "finance.h"
#include "storage.h"
#include "auth.h"
#include "utils.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* =========================================================
 *  Check-in
 * ========================================================= */
void checkin_do(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("CHECK-IN");

    int id = input_int("  ID da reserva", 1, 999999);
    Reservation *r = res_find_by_id(id);
    if (!r) {
        printf("  [!] Reserva nao encontrada.\n");
        pause_screen();
        return;
    }
    if (r->status != RES_CONFIRMADA && r->status != RES_PENDENTE) {
        printf("  [!] Reserva com status '%s' nao pode fazer check-in.\n",
               res_status_str(r->status));
        pause_screen();
        return;
    }

    Guest *g = guest_find_by_id(r->guest_id);
    res_print_full(r);

    /* ── Verificação de documento ── */
    printf("  Documento apresentado pelo hospede: ");
    char doc_check[20];
    input_string("", doc_check, sizeof(doc_check));
    if (g && !str_contains(g->document, doc_check) && !str_contains(doc_check, "ok")) {
        printf("  [!] Documento divergente. Registrado: %s\n",
               g ? g->document : "?");
        if (!input_confirm("  Prosseguir mesmo assim")) {
            pause_screen();
            return;
        }
    }

    /* ── Verificação de early check-in ── */
    time_t now    = time(NULL);
    int    early  = 0;
    struct tm *tm_ci      = localtime(&r->check_in_date);
    int         ci_day    = tm_ci->tm_yday;
    struct tm *tm_now     = localtime(&now);
    int         today_day = tm_now->tm_yday;

    if (today_day < ci_day) {
        printf("\n  [!] CHECK-IN ANTECIPADO — data prevista: ");
        char ci_str[14];
        date_to_string(r->check_in_date, ci_str);
        printf("%s\n", ci_str);
        printf("  Taxa de early check-in: R$ %.2f\n", g_config.early_checkin_fee);
        if (input_confirm("  Cobrar taxa e prosseguir")) {
            early = 1;
        } else {
            pause_screen();
            return;
        }
    }

    /* ── Caução ── */
    double caucao = 0.0;
    if (input_confirm("\n  Registrar caucao/deposito de seguranca")) {
        caucao = input_double("  Valor da caucao (R$)", 0.0, 99999.99);
        printf("  Caucao de R$ %.2f registrada.\n", caucao);
    }

    /* ── Acompanhantes extras ── */
    if (input_confirm("  Adicionar acompanhantes nao previstos")) {
        int extra_adults   = input_int("  Adultos extras", 0, 10);
        int extra_children = input_int("  Criancas extras", 0, 10);
        r->adults   += extra_adults;
        r->children += extra_children;
    }

    /* ── Conclusão ── */
    r->status         = RES_CHECKIN_FEITO;
    r->actual_check_in = now;

    if (early) {
        /* Registra taxa como ServiceItem */
        if (g_service_count < MAX_SERVICES) {
            ServiceItem si;
            memset(&si, 0, sizeof(si));
            si.id             = storage_next_service_id();
            si.reservation_id = r->id;
            si.category       = SVC_OUTRO;
            strncpy(si.description, "Taxa de early check-in",
                    sizeof(si.description) - 1);
            si.unit_price     = g_config.early_checkin_fee;
            si.quantity       = 1;
            si.status         = SVC_STATUS_ENTREGUE;
            si.registered_at  = now;
            si.registered_by  = g_session.staff.id;
            g_services[g_service_count++] = si;
            r->services_total += si.unit_price;
        }
    }

    /* Marca quartos como OCUPADO */
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        if (rm) {
            rm->status       = ROOM_OCUPADO;
            rm->clean_status = CLEAN_LIMPO;
        }
    }

    storage_save_rooms();
    storage_save_reservations();
    storage_save_services();

    char log[80];
    snprintf(log, sizeof(log), "CHECKIN id=%d guest=%d caucao=%.2f",
             r->id, r->guest_id, caucao);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Check-in realizado! Quarto(s): ");
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        printf("%s ", rm ? rm->number : "?");
    }
    printf("\n  Boas-vindas, %s!\n", g ? g->full_name : "Hospede");
    pause_screen();
}

/* =========================================================
 *  Check-out
 * ========================================================= */
void checkout_do(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("CHECK-OUT");

    int id = input_int("  ID da reserva", 1, 999999);
    Reservation *r = res_find_by_id(id);
    if (!r) {
        printf("  [!] Reserva nao encontrada.\n");
        pause_screen();
        return;
    }
    if (r->status != RES_CHECKIN_FEITO) {
        printf("  [!] Reserva com status '%s' nao pode fazer check-out.\n",
               res_status_str(r->status));
        pause_screen();
        return;
    }

    time_t now = time(NULL);

    /* ── Verificação de late check-out ── */
    struct tm *tm_co       = localtime(&r->check_out_date);
    int         co_day     = tm_co->tm_yday;
    int         co_year    = tm_co->tm_year;
    struct tm *tm_now      = localtime(&now);
    int         today_day  = tm_now->tm_yday;
    int         today_year = tm_now->tm_year;

    if (today_year > co_year || today_day > co_day) {
        int extra_days = date_diff_days(r->check_out_date, now);
        if (extra_days < 1) extra_days = 1;
        printf("\n  [!] LATE CHECK-OUT — %d dia(s) extra(s).\n", extra_days);

        double late_fee = 0.0;
        /* até 6h extra = taxa flat; mais de 6h = diária completa por quarto */
        struct tm *tm_extra = localtime(&now);
        if (tm_extra->tm_hour <= 18) {
            late_fee = g_config.late_checkout_fee;
            printf("  Taxa late check-out: R$ %.2f\n", late_fee);
        } else {
            /* cobra diária completa por dia extra, por quarto */
            for (int i = 0; i < r->room_count; i++) {
                Room *rm = room_find_by_id(r->room_ids[i]);
                if (rm) late_fee += rm->daily_rate * extra_days;
            }
            printf("  Diaria(s) extra(s): R$ %.2f\n", late_fee);
        }

        if (late_fee > 0.0 && g_service_count < MAX_SERVICES) {
            ServiceItem si;
            memset(&si, 0, sizeof(si));
            si.id             = storage_next_service_id();
            si.reservation_id = r->id;
            si.category       = SVC_OUTRO;
            snprintf(si.description, sizeof(si.description),
                     "Late check-out (%d dia(s) extra)", extra_days);
            si.unit_price     = late_fee;
            si.quantity       = 1;
            si.status         = SVC_STATUS_ENTREGUE;
            si.registered_at  = now;
            si.registered_by  = g_session.staff.id;
            g_services[g_service_count++] = si;
            r->services_total += late_fee;
        }
    }

    /* ── Recalcula total com todos os serviços acumulados ── */
    r->services_total = 0.0;
    for (int i = 0; i < g_service_count; i++) {
        if (g_services[i].reservation_id == r->id
            && g_services[i].status != SVC_STATUS_CANCELADO)
        {
            r->services_total += g_services[i].unit_price
                                 * g_services[i].quantity;
        }
    }
    r->base_value  = res_calc_base_value(r);
    r->total_value = r->base_value + r->services_total - r->discount;
    if (r->total_value < 0.0) r->total_value = 0.0;

    /* ── Exibe extrato ── */
    printf("\n");
    print_separator('=', 60);
    printf("  EXTRATO FINAL — Reserva %s\n", r->code);
    print_separator('=', 60);
    Guest *g = guest_find_by_id(r->guest_id);
    printf("  Hospede   : %s\n", g ? g->full_name : "?");

    char ci_str[14], co_str[14];
    date_to_string(r->check_in_date,  ci_str);
    date_to_string(r->check_out_date, co_str);
    int nights = date_diff_days(r->check_in_date, r->check_out_date);
    if (nights <= 0) nights = 1;

    printf("  Periodo   : %s a %s (%d noite(s))\n", ci_str, co_str, nights);
    printf("  Quartos   : ");
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        printf("%s ", rm ? rm->number : "?");
    }
    printf("\n\n");
    printf("  Diarias   : R$ %8.2f\n", r->base_value);

    if (g_service_count > 0) {
        printf("  Servicos:\n");
        for (int i = 0; i < g_service_count; i++) {
            ServiceItem *si = &g_services[i];
            if (si->reservation_id != r->id
                || si->status == SVC_STATUS_CANCELADO) continue;
            printf("    %-35s R$ %6.2f x%d\n",
                   si->description, si->unit_price, si->quantity);
        }
    }

    if (r->discount > 0.0)
        printf("  Desconto  : R$ %8.2f\n", r->discount);
    print_separator('-', 40);
    printf("  TOTAL     : R$ %8.2f\n\n", r->total_value);

    /* ── Pagamento ── */
    printf("  Forma de pagamento:\n");
    printf("  [0]Dinheiro [1]Credito [2]Debito [3]PIX [4]Transferencia [5]Faturado\n");
    r->payment_method = (PaymentMethod)input_int("  Opcao", 0, 5);
    if (r->payment_method == PAY_CREDITO) {
        r->payment_installments = input_int("  Parcelas", 1, 12);
    }

    if (r->discount == 0.0 && input_confirm("  Aplicar desconto adicional")) {
        double disc = input_double("  Valor do desconto (R$)", 0.0, r->total_value);
        r->discount    += disc;
        r->total_value -= disc;
        if (r->total_value < 0.0) r->total_value = 0.0;
    }

    if (!input_confirm("\n  Confirmar pagamento e encerrar")) {
        pause_screen();
        return;
    }

    /* ── Conclusão ── */
    r->status           = RES_CHECKOUT_FEITO;
    r->actual_check_out = now;

    /* Libera quartos → sujos */
    for (int i = 0; i < r->room_count; i++) {
        Room *rm = room_find_by_id(r->room_ids[i]);
        if (rm) {
            rm->status       = ROOM_LIVRE;
            rm->clean_status = CLEAN_SUJO;
        }
    }

    /* Atualiza fidelidade */
    guest_update_fidelity(r->guest_id, r->total_value);

    storage_save_rooms();
    storage_save_reservations();
    storage_save_services();

    char log[100];
    snprintf(log, sizeof(log), "CHECKOUT id=%d total=%.2f pay=%d",
             r->id, r->total_value, (int)r->payment_method);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Check-out concluido. Total cobrado: R$ %.2f\n", r->total_value);
    printf("  Obrigado pela hospedagem, %s!\n", g ? g->full_name : "Hospede");
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static const MenuItem checkin_items[] = {
    {1, "Realizar Check-in",   ACCESS_OPERADOR, checkin_do},
    {2, "Realizar Check-out",  ACCESS_OPERADOR, checkout_do},
    {0, "Voltar",              0,               NULL}
};

void checkin_menu(void) {
    menu_run("MODULO: CHECK-IN / CHECK-OUT",
             checkin_items,
             (int)(sizeof(checkin_items) / sizeof(checkin_items[0])));
}
