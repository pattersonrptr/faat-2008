#ifndef HOUSEKEEPING_H
#define HOUSEKEEPING_H

/* ── Limpeza ──────────────────────────────────────────── */
void hk_daily_list(void);           /* lista de quartos para limpar hoje     */
void hk_update_clean_status(void);  /* atualiza status de limpeza de um quarto */
void hk_assign_room(void);          /* atribui camareiro a um quarto         */

/* ── Objetos esquecidos ───────────────────────────────── */
void hk_lost_item_add(void);
void hk_lost_item_list(void);
void hk_lost_item_resolve(void);

/* ── Ponto de entrada do módulo ───────────────────────── */
void hk_menu(void);

#endif /* HOUSEKEEPING_H */
