#include <stdio.h>

#include "types.h"
#include "utils.h"
#include "storage.h"
#include "auth.h"
#include "menu.h"
#include "rooms.h"
#include "guests.h"
#include "reservations.h"
#include "checkin.h"
#include "finance.h"
#include "services.h"
#include "staff.h"
#include "housekeeping.h"
#include "reports.h"

static const MenuItem main_items[] = {
    {1, "Quartos",              ACCESS_BASICO,   room_menu},
    {2, "Hospedes",             ACCESS_BASICO,   guest_menu},
    {3, "Reservas",             ACCESS_BASICO,   res_menu},
    {4, "Check-in / Check-out", ACCESS_OPERADOR, checkin_menu},
    {5, "Servicos",             ACCESS_OPERADOR, svc_menu},
    {6, "Financeiro",           ACCESS_OPERADOR, finance_menu},
    {7, "Governanca",           ACCESS_OPERADOR, hk_menu},
    {8, "Funcionarios",         ACCESS_GERENTE,  staff_menu},
    {9, "Relatorios",           ACCESS_BASICO,   report_menu},
    {0, "Sair",                 0,               NULL}
};

int main(void) {
    storage_init();

    if (!auth_login()) {
        printf("\n  Acesso bloqueado. Encerrando.\n");
        storage_save_all();
        return 1;
    }

    menu_run(g_config.hotel_name,
             main_items,
             (int)(sizeof(main_items) / sizeof(main_items[0])));

    auth_logout();
    storage_save_all();
    printf("\n  Sistema encerrado. Ate logo!\n\n");
    return 0;
}
