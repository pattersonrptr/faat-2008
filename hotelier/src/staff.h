#ifndef STAFF_H
#define STAFF_H

#include "types.h"

/* ── Helpers ──────────────────────────────────────────── */
const char *staff_role_str(StaffRole r);
const char *staff_shift_str(int shift);

/* ── Busca ────────────────────────────────────────────── */
Staff *staff_find_by_id(int id);
Staff *staff_find_by_username(const char *username);

/* ── Exibição ─────────────────────────────────────────── */
void   staff_print_brief(const Staff *s);
void   staff_print_full(const Staff *s);
void   staff_list(void);

/* ── CRUD e operações ─────────────────────────────────── */
void   staff_add(void);
void   staff_edit(void);
void   staff_change_password(void);
void   staff_toggle_active(void);

/* ── Ponto de entrada do módulo ───────────────────────── */
void   staff_menu(void);

#endif /* STAFF_H */
