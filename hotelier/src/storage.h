#ifndef STORAGE_H
#define STORAGE_H

#include "types.h"

/* =========================================================
 *  Banco de dados em memória (arrays estáticos globais)
 *  Declarados em storage.c, acessíveis via extern aqui.
 * ========================================================= */
extern Room         g_rooms[MAX_ROOMS];
extern int          g_room_count;

extern Guest        g_guests[MAX_GUESTS];
extern int          g_guest_count;

extern Reservation  g_reservations[MAX_RESERVATIONS];
extern int          g_reservation_count;

extern ServiceItem  g_services[MAX_SERVICES];
extern int          g_service_count;

extern Staff        g_staff[MAX_STAFF];
extern int          g_staff_count;

extern LostItem     g_lost_items[MAX_LOST_ITEMS];
extern int          g_lost_item_count;

extern HotelConfig  g_config;

extern CashShift    g_shifts[MAX_SHIFTS];
extern int          g_shift_count;

/* =========================================================
 *  Caminhos dos arquivos de dados
 * ========================================================= */
#define PATH_ROOMS        "data/rooms.dat"
#define PATH_GUESTS       "data/guests.dat"
#define PATH_RESERVATIONS "data/reservations.dat"
#define PATH_SERVICES     "data/services.dat"
#define PATH_STAFF        "data/staff.dat"
#define PATH_LOST_ITEMS   "data/lost_items.dat"
#define PATH_CONFIG       "data/config.dat"
#define PATH_AUDIT_LOG    "data/audit.log"
#define PATH_SHIFTS       "data/shifts.dat"

/* =========================================================
 *  Inicialização e encerramento
 * ========================================================= */
void storage_init(void);    /* carrega todos os arquivos na memória */
void storage_save_all(void); /* grava tudo em disco */

/* =========================================================
 *  Operações por entidade
 *  Convenção de retorno: 0 = sucesso, -1 = erro
 * ========================================================= */

/* --- Quartos --- */
int  storage_save_rooms(void);
int  storage_load_rooms(void);
int  storage_next_room_id(void);

/* --- Hóspedes --- */
int  storage_save_guests(void);
int  storage_load_guests(void);
int  storage_next_guest_id(void);

/* --- Reservas --- */
int  storage_save_reservations(void);
int  storage_load_reservations(void);
int  storage_next_reservation_id(void);

/* --- Serviços --- */
int  storage_save_services(void);
int  storage_load_services(void);
int  storage_next_service_id(void);

/* --- Funcionários --- */
int  storage_save_staff(void);
int  storage_load_staff(void);
int  storage_next_staff_id(void);

/* --- Objetos esquecidos --- */
int  storage_save_lost_items(void);
int  storage_load_lost_items(void);
int  storage_next_lost_item_id(void);

/* --- Configuração --- */
int  storage_save_config(void);
int  storage_load_config(void);

/* --- Turnos de caixa --- */
int  storage_save_shifts(void);
int  storage_load_shifts(void);
int  storage_next_shift_id(void);

/* --- Backup --- */
int  storage_backup(char *out_path, size_t out_size);

/* --- Auditoria --- */
void storage_audit_log(int staff_id, const char *action);

#endif /* STORAGE_H */
