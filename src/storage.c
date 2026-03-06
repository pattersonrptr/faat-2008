#include "storage.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* =========================================================
 *  Arrays globais (banco de dados em memória)
 * ========================================================= */
Room        g_rooms[MAX_ROOMS];
int         g_room_count = 0;

Guest       g_guests[MAX_GUESTS];
int         g_guest_count = 0;

Reservation g_reservations[MAX_RESERVATIONS];
int         g_reservation_count = 0;

ServiceItem g_services[MAX_SERVICES];
int         g_service_count = 0;

Staff       g_staff[MAX_STAFF];
int         g_staff_count = 0;

LostItem    g_lost_items[MAX_LOST_ITEMS];
int         g_lost_item_count = 0;

HotelConfig g_config;

/* =========================================================
 *  Macro auxiliar para leitura/escrita binária genérica
 * ========================================================= */
#define WRITE_ARRAY(path, arr, count, type)                         \
    do {                                                            \
        FILE *f = fopen(path, "wb");                                \
        if (!f) { perror("storage: fopen " path); return -1; }     \
        fwrite(&(count), sizeof(int), 1, f);                        \
        if (count > 0) fwrite(arr, sizeof(type), (size_t)(count), f); \
        fclose(f);                                                  \
        return 0;                                                   \
    } while (0)

#define READ_ARRAY(path, arr, count, max, type)                     \
    do {                                                            \
        FILE *f = fopen(path, "rb");                                \
        if (!f) { count = 0; return 0; } /* arquivo ainda não existe */ \
        if (fread(&(count), sizeof(int), 1, f) != 1) { count = 0; fclose(f); return -1; } \
        if (count < 0 || count > max) { count = 0; fclose(f); return -1; } \
        if (count > 0) {                                            \
            size_t n = fread(arr, sizeof(type), (size_t)(count), f); \
            if ((int)n != count) { count = 0; fclose(f); return -1; } \
        }                                                           \
        fclose(f);                                                  \
        return 0;                                                   \
    } while (0)

/* =========================================================
 *  Configuração padrão (usada na primeira execução)
 * ========================================================= */
static void config_set_defaults(void) {
    memset(&g_config, 0, sizeof(g_config));
    strncpy(g_config.hotel_name,     "Hotel Sistema",        sizeof(g_config.hotel_name) - 1);
    strncpy(g_config.address,        "Rua Exemplo, 100",     sizeof(g_config.address) - 1);
    strncpy(g_config.cnpj,           "00.000.000/0001-00",   sizeof(g_config.cnpj) - 1);
    g_config.stars               = 3;
    snprintf(g_config.checkin_time,  sizeof(g_config.checkin_time),  "14:00");
    snprintf(g_config.checkout_time, sizeof(g_config.checkout_time), "12:00");
    g_config.tourist_tax         = 0.0;
    g_config.service_fee_pct     = 0.10;   /* 10% */
    g_config.late_checkout_fee   = 0.50;   /* 50% da diária */
    g_config.early_checkin_fee   = 0.50;
    g_config.cancel_free_days    = 3;
    g_config.cancel_partial_pct  = 0.50;
    g_config.points_per_real     = 1.0;
    g_config.multiplier_prata    = 1.5;
    g_config.multiplier_ouro     = 2.0;
    g_config.multiplier_diamante = 3.0;
}

/* =========================================================
 *  Criação do admin padrão (somente se staff vazio)
 * ========================================================= */
static void staff_create_default_admin(void) {
    if (g_staff_count > 0) return;

    Staff *a = &g_staff[0];
    memset(a, 0, sizeof(Staff));
    a->id           = 1;
    strncpy(a->full_name, "Administrador", sizeof(a->full_name) - 1);
    strncpy(a->document,  "00000000000",   sizeof(a->document) - 1);
    a->role         = ROLE_GERENTE;
    strncpy(a->department, "Gerencia",     sizeof(a->department) - 1);
    a->access_level = ACCESS_ADMIN;
    strncpy(a->username, "admin",          sizeof(a->username) - 1);
    /* Senha padrão: "admin123" */
    unsigned long h = hash_djb2("admin123");
    hash_to_hex(h, a->password_hash);
    a->active       = 1;
    snprintf(a->hire_date, sizeof(a->hire_date), "01/01/2026");
    g_staff_count   = 1;
    storage_save_staff();
}

/* =========================================================
 *  Inicialização e encerramento
 * ========================================================= */
void storage_init(void) {
    storage_load_rooms();
    storage_load_guests();
    storage_load_reservations();
    storage_load_services();
    storage_load_staff();
    storage_load_lost_items();

    if (storage_load_config() != 0) {
        config_set_defaults();
        storage_save_config();
    }

    staff_create_default_admin();
}

void storage_save_all(void) {
    storage_save_rooms();
    storage_save_guests();
    storage_save_reservations();
    storage_save_services();
    storage_save_staff();
    storage_save_lost_items();
    storage_save_config();
}

/* =========================================================
 *  Quartos
 * ========================================================= */
int storage_save_rooms(void)  { WRITE_ARRAY(PATH_ROOMS, g_rooms, g_room_count, Room); }
int storage_load_rooms(void)  { READ_ARRAY(PATH_ROOMS, g_rooms, g_room_count, MAX_ROOMS, Room); }

int storage_next_room_id(void) {
    int max = 0;
    for (int i = 0; i < g_room_count; i++) {
        if (g_rooms[i].id > max) max = g_rooms[i].id;
    }
    return max + 1;
}

/* =========================================================
 *  Hóspedes
 * ========================================================= */
int storage_save_guests(void)  { WRITE_ARRAY(PATH_GUESTS, g_guests, g_guest_count, Guest); }
int storage_load_guests(void)  { READ_ARRAY(PATH_GUESTS, g_guests, g_guest_count, MAX_GUESTS, Guest); }

int storage_next_guest_id(void) {
    int max = 0;
    for (int i = 0; i < g_guest_count; i++) {
        if (g_guests[i].id > max) max = g_guests[i].id;
    }
    return max + 1;
}

/* =========================================================
 *  Reservas
 * ========================================================= */
int storage_save_reservations(void) { WRITE_ARRAY(PATH_RESERVATIONS, g_reservations, g_reservation_count, Reservation); }
int storage_load_reservations(void) { READ_ARRAY(PATH_RESERVATIONS, g_reservations, g_reservation_count, MAX_RESERVATIONS, Reservation); }

int storage_next_reservation_id(void) {
    int max = 0;
    for (int i = 0; i < g_reservation_count; i++) {
        if (g_reservations[i].id > max) max = g_reservations[i].id;
    }
    return max + 1;
}

/* =========================================================
 *  Serviços
 * ========================================================= */
int storage_save_services(void)  { WRITE_ARRAY(PATH_SERVICES, g_services, g_service_count, ServiceItem); }
int storage_load_services(void)  { READ_ARRAY(PATH_SERVICES, g_services, g_service_count, MAX_SERVICES, ServiceItem); }

int storage_next_service_id(void) {
    int max = 0;
    for (int i = 0; i < g_service_count; i++) {
        if (g_services[i].id > max) max = g_services[i].id;
    }
    return max + 1;
}

/* =========================================================
 *  Funcionários
 * ========================================================= */
int storage_save_staff(void)  { WRITE_ARRAY(PATH_STAFF, g_staff, g_staff_count, Staff); }
int storage_load_staff(void)  { READ_ARRAY(PATH_STAFF, g_staff, g_staff_count, MAX_STAFF, Staff); }

int storage_next_staff_id(void) {
    int max = 0;
    for (int i = 0; i < g_staff_count; i++) {
        if (g_staff[i].id > max) max = g_staff[i].id;
    }
    return max + 1;
}

/* =========================================================
 *  Objetos esquecidos
 * ========================================================= */
int storage_save_lost_items(void) { WRITE_ARRAY(PATH_LOST_ITEMS, g_lost_items, g_lost_item_count, LostItem); }
int storage_load_lost_items(void) { READ_ARRAY(PATH_LOST_ITEMS, g_lost_items, g_lost_item_count, MAX_LOST_ITEMS, LostItem); }

int storage_next_lost_item_id(void) {
    int max = 0;
    for (int i = 0; i < g_lost_item_count; i++) {
        if (g_lost_items[i].id > max) max = g_lost_items[i].id;
    }
    return max + 1;
}

/* =========================================================
 *  Configuração
 * ========================================================= */
int storage_save_config(void) {
    FILE *f = fopen(PATH_CONFIG, "wb");
    if (!f) { perror("storage: fopen config"); return -1; }
    fwrite(&g_config, sizeof(HotelConfig), 1, f);
    fclose(f);
    return 0;
}

int storage_load_config(void) {
    FILE *f = fopen(PATH_CONFIG, "rb");
    if (!f) return -1;
    size_t n = fread(&g_config, sizeof(HotelConfig), 1, f);
    fclose(f);
    return (n == 1) ? 0 : -1;
}

/* =========================================================
 *  Auditoria
 * ========================================================= */
void storage_audit_log(int staff_id, const char *action) {
    FILE *f = fopen(PATH_AUDIT_LOG, "a");
    if (!f) return;

    time_t now = time(NULL);
    char   dt[20];
    datetime_to_string(now, dt);
    fprintf(f, "[%s] staff_id=%d | %s\n", dt, staff_id, action);
    fclose(f);
}
