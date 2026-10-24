#ifndef ROOMS_H
#define ROOMS_H

#include "types.h"

/* ── Helpers de conversão ─────────────────────────────── */
const char *room_type_str(RoomType t);
const char *room_status_str(RoomStatus s);
const char *room_clean_str(CleanStatus s);
void        room_features_str(int features, char *buf, int buf_size);

/* ── Busca ────────────────────────────────────────────── */
Room *room_find_by_id(int id);
Room *room_find_by_number(const char *number); /* número já em maiúsculo */

/* ── Exibição ─────────────────────────────────────────── */
void  room_print_brief(const Room *r);          /* linha única na tabela     */
void  room_print_full(const Room *r);           /* ficha completa do quarto  */
void  room_list(int status_filter);             /* -1 = todos os status      */
void  room_occupation_map(void);                /* mapa visual por andar     */

/* ── CRUD e operações ─────────────────────────────────── */
void  room_add(void);
void  room_edit(void);
void  room_change_status(void);
void  room_add_maintenance(void);
void  room_maintenance_history(void);

/* ── Ponto de entrada do módulo ───────────────────────── */
void  room_menu(void);

#endif /* ROOMS_H */
