#ifndef GUESTS_H
#define GUESTS_H

#include "types.h"

/* ── Helpers ──────────────────────────────────────────── */
const char *guest_type_str(GuestType t);
const char *guest_fidelity_str(FidelityLevel l);

/* ── Busca ────────────────────────────────────────────── */
Guest *guest_find_by_id(int id);
Guest *guest_find_by_document(const char *doc);

/* ── Exibição ─────────────────────────────────────────── */
void   guest_print_brief(const Guest *g);
void   guest_print_full(const Guest *g);
void   guest_list(void);
void   guest_search(void);

/* ── CRUD e operações ─────────────────────────────────── */
void   guest_add(void);
void   guest_edit(void);
void   guest_toggle_blacklist(void);

/* Atualiza pontos/nível de fidelidade; chamado pelo módulo de checkout */
void   guest_update_fidelity(int guest_id, double amount_spent);

/* ── Ponto de entrada do módulo ───────────────────────── */
void   guest_menu(void);

#endif /* GUESTS_H */
