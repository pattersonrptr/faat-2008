#include "services.h"
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
const char *svc_category_str(ServiceCategory c) {
    switch (c) {
        case SVC_FRIGOBAR:       return "Frigobar";
        case SVC_ROOM_SERVICE:   return "Room Service";
        case SVC_LAVANDERIA:     return "Lavanderia";
        case SVC_ESTACIONAMENTO: return "Estacionamento";
        case SVC_TELEFONIA:      return "Telefonia";
        case SVC_OUTRO:          return "Outro";
        default:                 return "?";
    }
}

const char *svc_status_str(ServiceStatus s) {
    switch (s) {
        case SVC_STATUS_PENDENTE:  return "PENDENTE";
        case SVC_STATUS_ENTREGUE:  return "ENTREGUE";
        case SVC_STATUS_CANCELADO: return "CANCELADO";
        default:                   return "?";
    }
}

/* =========================================================
 *  Helper interno: busca reserva ativa por número de quarto
 * ========================================================= */
static Reservation *find_active_res_by_room_number(const char *room_number) {
    Room *rm = room_find_by_number(room_number);
    if (!rm) return NULL;
    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status != RES_CHECKIN_FEITO) continue;
        for (int j = 0; j < r->room_count; j++) {
            if (r->room_ids[j] == rm->id) return r;
        }
    }
    return NULL;
}

/* =========================================================
 *  Helper interno: cria e persiste um ServiceItem
 * ========================================================= */
static int svc_create(int res_id, ServiceCategory cat,
                      const char *desc, double unit_price,
                      int qty, ServiceStatus status)
{
    if (g_service_count >= MAX_SERVICES) {
        printf("  [!] Limite de servicos atingido.\n");
        return -1;
    }
    ServiceItem si;
    memset(&si, 0, sizeof(si));
    si.id             = storage_next_service_id();
    si.reservation_id = res_id;
    si.category       = cat;
    strncpy(si.description, desc, sizeof(si.description) - 1);
    si.unit_price    = unit_price;
    si.quantity      = qty;
    si.status        = status;
    si.registered_at = time(NULL);
    si.registered_by = g_session.staff.id;

    /* Atualiza total de serviços na reserva */
    Reservation *res = res_find_by_id(res_id);
    if (res && status != SVC_STATUS_CANCELADO)
        res->services_total += unit_price * qty;

    g_services[g_service_count++] = si;
    storage_save_services();
    storage_save_reservations();

    char log[120];
    snprintf(log, sizeof(log), "SVC ADD id=%d res=%d cat=%s val=%.2f",
             si.id, res_id, svc_category_str(cat), unit_price * qty);
    storage_audit_log(g_session.staff.id, log);
    return si.id;
}

/* =========================================================
 *  Helper: pede número do quarto e retorna reserva ativa
 * ========================================================= */
static Reservation *prompt_active_reservation(void) {
    char num[8];
    input_string("  Numero do quarto: ", num, sizeof(num));
    str_upper(num);
    Reservation *r = find_active_res_by_room_number(num);
    if (!r) {
        printf("  [!] Nenhuma hospedagem ativa no quarto '%s'.\n", num);
        pause_screen();
    }
    return r;
}

/* =========================================================
 *  Listagens
 * ========================================================= */
static void svc_print_header(void) {
    printf("  %-5s %-14s %-30s %-5s %-8s %-10s %s\n",
           "ID", "Categoria", "Descricao", "Qtd", "Unit.", "Total", "Status");
    print_separator('-', 85);
}

void svc_list_by_reservation(int reservation_id) {
    int found = 0;
    svc_print_header();
    for (int i = 0; i < g_service_count; i++) {
        ServiceItem *si = &g_services[i];
        if (si->reservation_id != reservation_id) continue;
        printf("  %-5d %-14s %-30s %-5d R$%-6.2f R$%-8.2f %s\n",
               si->id,
               svc_category_str(si->category),
               si->description,
               si->quantity,
               si->unit_price,
               si->unit_price * si->quantity,
               svc_status_str(si->status));
        found++;
    }
    print_separator('-', 85);
    printf("  %d servico(s) lancado(s).\n", found);
}

void svc_list_pending(void) {
    clear_screen();
    print_header("SERVICOS PENDENTES");
    int found = 0;
    svc_print_header();
    for (int i = 0; i < g_service_count; i++) {
        ServiceItem *si = &g_services[i];
        if (si->status != SVC_STATUS_PENDENTE) continue;
        printf("  %-5d %-14s %-30s %-5d R$%-6.2f R$%-8.2f %s\n",
               si->id,
               svc_category_str(si->category),
               si->description,
               si->quantity,
               si->unit_price,
               si->unit_price * si->quantity,
               svc_status_str(si->status));
        found++;
    }
    print_separator('-', 85);
    printf("  %d pendente(s).\n", found);
    pause_screen();
}

/* =========================================================
 *  Lançamento por categoria
 * ========================================================= */
void svc_add_frigobar(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;
    clear_screen();
    print_header("LANCAMENTO — FRIGOBAR");

    Reservation *r = prompt_active_reservation();
    if (!r) return;

    printf("  Reserva: %s  |  Hospede ID: %d\n\n", r->code, r->guest_id);

    int more = 1;
    while (more) {
        char item[100];
        input_string("  Item consumido: ", item, sizeof(item));
        double price = input_double("  Preco unitario (R$)", 0.01, 9999.99);
        int    qty   = input_int("  Quantidade", 1, 99);

        svc_create(r->id, SVC_FRIGOBAR, item, price, qty, SVC_STATUS_ENTREGUE);
        printf("  [OK] Lancado: %dx %s = R$ %.2f\n", qty, item, price * qty);

        more = input_confirm("  Adicionar outro item");
    }
    pause_screen();
}

void svc_add_room_service(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;
    clear_screen();
    print_header("LANCAMENTO — ROOM SERVICE");

    Reservation *r = prompt_active_reservation();
    if (!r) return;

    printf("  Reserva: %s  |  Hospede ID: %d\n\n", r->code, r->guest_id);

    char desc[200];
    input_string("  Descricao do pedido: ", desc, sizeof(desc));
    double price = input_double("  Valor total do pedido (R$)", 0.01, 99999.99);

    printf("  Status do pedido:\n");
    printf("  [0] Pendente (em preparo)   [1] Entregue\n");
    int st = input_int("  Status", 0, 1);

    int svc_id = svc_create(r->id, SVC_ROOM_SERVICE, desc, price, 1,
                            st == 0 ? SVC_STATUS_PENDENTE : SVC_STATUS_ENTREGUE);
    if (svc_id >= 0)
        printf("\n  [OK] Room service registrado (ID servico: %d).\n", svc_id);
    pause_screen();
}

void svc_add_laundry(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;
    clear_screen();
    print_header("LANCAMENTO — LAVANDERIA");

    Reservation *r = prompt_active_reservation();
    if (!r) return;

    printf("  Reserva: %s  |  Hospede ID: %d\n\n", r->code, r->guest_id);

    char desc[200];
    printf("  Tipo: [1] Lavar/Passar   [2] A seco\n");
    int tipo = input_int("  Tipo", 1, 2);
    input_string("  Descricao das pecas: ", desc, sizeof(desc));

    char full_desc[220];
    snprintf(full_desc, sizeof(full_desc), "%s | %s",
             tipo == 1 ? "Lavar/Passar" : "A seco", desc);

    double price = input_double("  Valor (R$)", 0.01, 9999.99);
    int    qty   = input_int("  Numero de pecas", 1, 99);

    int st = input_confirm("  Ja entregue") ? SVC_STATUS_ENTREGUE : SVC_STATUS_PENDENTE;

    int svc_id = svc_create(r->id, SVC_LAVANDERIA, full_desc, price, qty, (ServiceStatus)st);
    if (svc_id >= 0)
        printf("\n  [OK] Lavanderia registrada (ID: %d).\n", svc_id);
    pause_screen();
}

void svc_add_parking(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;
    clear_screen();
    print_header("LANCAMENTO — ESTACIONAMENTO");

    Reservation *r = prompt_active_reservation();
    if (!r) return;

    char plate[10], model[40];
    input_string("  Placa do veiculo : ", plate, sizeof(plate));
    str_upper(plate);
    input_string("  Modelo/cor       : ", model, sizeof(model));

    printf("  Cobranca: [1] Por hora   [2] Diaria\n");
    int tipo = input_int("  Tipo", 1, 2);

    double rate, qty;
    if (tipo == 1) {
        rate = input_double("  Valor por hora (R$)", 0.01, 999.99);
        qty  = input_double("  Horas (pode ser fracionado)", 0.5, 720.0);
    } else {
        rate = input_double("  Valor por diaria (R$)", 0.01, 999.99);
        qty  = (double)input_int("  Dias", 1, 60);
    }

    char desc[200];
    snprintf(desc, sizeof(desc), "Estacionamento %s %s (%.1f %s)",
             plate, model, qty, tipo == 1 ? "h" : "dia(s)");

    int svc_id = svc_create(r->id, SVC_ESTACIONAMENTO, desc, rate, (int)qty,
                            SVC_STATUS_ENTREGUE);
    if (svc_id >= 0)
        printf("\n  [OK] Estacionamento registrado. Total: R$ %.2f\n", rate * qty);
    pause_screen();
}

void svc_add_other(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;
    clear_screen();
    print_header("LANCAMENTO — OUTRO SERVICO");

    Reservation *r = prompt_active_reservation();
    if (!r) return;

    char desc[200];
    input_string("  Descricao: ", desc, sizeof(desc));
    double price = input_double("  Valor unitario (R$)", 0.01, 99999.99);
    int    qty   = input_int("  Quantidade", 1, 999);

    printf("  Categoria:\n");
    printf("  [4] Telefonia   [5] Outro\n");
    int cat = input_int("  Categoria", 4, 5);

    int svc_id = svc_create(r->id, (ServiceCategory)cat, desc, price, qty,
                            SVC_STATUS_ENTREGUE);
    if (svc_id >= 0)
        printf("\n  [OK] Servico registrado. Total: R$ %.2f\n", price * qty);
    pause_screen();
}

/* =========================================================
 *  Atualizar status de serviço
 * ========================================================= */
void svc_update_status(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;
    clear_screen();
    print_header("ATUALIZAR STATUS DE SERVICO");

    int sid = input_int("  ID do servico", 1, 999999);

    /* Busca linear */
    ServiceItem *si = NULL;
    for (int i = 0; i < g_service_count; i++) {
        if (g_services[i].id == sid) { si = &g_services[i]; break; }
    }
    if (!si) {
        printf("  [!] Servico ID %d nao encontrado.\n", sid);
        pause_screen();
        return;
    }

    printf("  Servico: [%d] %s | Status atual: %s\n",
           si->id, si->description, svc_status_str(si->status));
    printf("  Novo status: [0] Pendente   [1] Entregue   [2] Cancelado\n");
    ServiceStatus novo = (ServiceStatus)input_int("  Opcao", 0, 2);

    /* Ajusta total da reserva se cancelando */
    if (novo == SVC_STATUS_CANCELADO && si->status != SVC_STATUS_CANCELADO) {
        Reservation *res = res_find_by_id(si->reservation_id);
        if (res) {
            res->services_total -= si->unit_price * si->quantity;
            if (res->services_total < 0.0) res->services_total = 0.0;
            storage_save_reservations();
        }
    }

    si->status = novo;
    storage_save_services();

    char log[80];
    snprintf(log, sizeof(log), "SVC STATUS id=%d -> %s", si->id, svc_status_str(novo));
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Status atualizado para: %s\n", svc_status_str(novo));
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static void svc_list_res_wrap(void) {
    clear_screen();
    print_header("SERVICOS POR RESERVA");
    int id = input_int("  ID da reserva", 1, 999999);
    svc_list_by_reservation(id);
    pause_screen();
}

static const MenuItem svc_items[] = {
    {1, "Servicos por reserva",     ACCESS_BASICO,   svc_list_res_wrap},
    {2, "Servicos pendentes",       ACCESS_BASICO,   svc_list_pending},
    {3, "Lancamento — Frigobar",    ACCESS_OPERADOR, svc_add_frigobar},
    {4, "Lancamento — Room Service",ACCESS_OPERADOR, svc_add_room_service},
    {5, "Lancamento — Lavanderia",  ACCESS_OPERADOR, svc_add_laundry},
    {6, "Lancamento — Estacionam.", ACCESS_OPERADOR, svc_add_parking},
    {7, "Lancamento — Outro",       ACCESS_OPERADOR, svc_add_other},
    {8, "Atualizar status servico", ACCESS_OPERADOR, svc_update_status},
    {0, "Voltar",                   0,               NULL}
};

void svc_menu(void) {
    menu_run("MODULO: SERVICOS",
             svc_items,
             (int)(sizeof(svc_items) / sizeof(svc_items[0])));
}
