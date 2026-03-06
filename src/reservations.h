#ifndef RESERVATIONS_H
#define RESERVATIONS_H

#include "types.h"

/* ── Helpers ──────────────────────────────────────────── */
const char *res_status_str(ReservationStatus s);

/* ── Busca ────────────────────────────────────────────── */
Reservation *res_find_by_id(int id);
Reservation *res_find_by_code(const char *code);

/* ── Listagens ────────────────────────────────────────── */
void res_print_brief(const Reservation *r);
void res_print_full(const Reservation *r);
void res_list(int status_filter);        /* -1 = todas */
void res_list_by_guest(int guest_id);
void res_search(void);

/* valor base (soma das diárias) sem serviços */
double res_calc_base_value(const Reservation *r);

/* ── CRUD e operações ─────────────────────────────────── */
void res_add(void);
void res_cancel(void);
void res_mark_noshow(void);

/* ── Ponto de entrada do módulo ───────────────────────── */
void res_menu(void);

#endif /* RESERVATIONS_H */
