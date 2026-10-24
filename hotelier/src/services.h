#ifndef SERVICES_H
#define SERVICES_H

#include "types.h"

/* ── Helpers ──────────────────────────────────────────── */
const char *svc_category_str(ServiceCategory c);
const char *svc_status_str(ServiceStatus s);

/* ── Listagens ────────────────────────────────────────── */
void svc_list_by_reservation(int reservation_id);
void svc_list_pending(void);

/* ── Lançamento por categoria ─────────────────────────── */
void svc_add_frigobar(void);
void svc_add_room_service(void);
void svc_add_laundry(void);
void svc_add_parking(void);
void svc_add_other(void);

/* Atualiza status de um serviço (pendente → entregue/cancelado) */
void svc_update_status(void);

/* ── Ponto de entrada do módulo ───────────────────────── */
void svc_menu(void);

#endif /* SERVICES_H */
