#include "reports.h"
#include "reservations.h"
#include "rooms.h"
#include "guests.h"
#include "finance.h"
#include "storage.h"
#include "auth.h"
#include "utils.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* =========================================================
 *  Taxa de ocupação por período
 * ========================================================= */
void report_occupation(void) {
    if (!auth_require(ACCESS_BASICO)) return;

    clear_screen();
    print_header("TAXA DE OCUPACAO");

    char from_str[12], to_str[12];
    do {
        input_string("  Data inicial (DD/MM/AAAA): ", from_str, sizeof(from_str));
        if (!date_is_valid(from_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(from_str));
    do {
        input_string("  Data final   (DD/MM/AAAA): ", to_str, sizeof(to_str));
        if (!date_is_valid(to_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(to_str));

    time_t t_from = date_from_string(from_str);
    time_t t_to   = date_from_string(to_str) + 86400;
    int    days   = date_diff_days(t_from, t_to);
    if (days <= 0) days = 1;

    if (g_room_count == 0) {
        printf("  Nenhum quarto cadastrado.\n");
        pause_screen();
        return;
    }

    /* Contabiliza noites ocupadas por tipo de quarto */
    long occ_nights[4]   = {0};
    long total_nights[4] = {0};

    /* Total de noites disponíveis por tipo */
    for (int i = 0; i < g_room_count; i++) {
        int t = (int)g_rooms[i].type;
        if (t < 0 || t > 3) t = 0;
        if (g_rooms[i].status != ROOM_BLOQUEADO)
            total_nights[t] += days;
    }

    /* Noites efetivamente ocupadas */
    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status == RES_CANCELADA || r->status == RES_NO_SHOW
            || r->status == RES_PENDENTE) continue;

        /* Interseção do período da reserva com o período consultado */
        time_t rs = r->check_in_date  > t_from ? r->check_in_date  : t_from;
        time_t re = r->check_out_date < t_to   ? r->check_out_date : t_to;
        int    n  = date_diff_days(rs, re);
        if (n <= 0) continue;

        for (int j = 0; j < r->room_count; j++) {
            Room *rm = room_find_by_id(r->room_ids[j]);
            if (!rm) continue;
            int t = (int)rm->type;
            if (t < 0 || t > 3) t = 0;
            occ_nights[t] += n;
        }
    }

    const char *type_names[] = {"Solteiro","Duplo","Suite","Presidencial"};
    long  total_occ = 0, total_avail = 0;

    printf("\n  %-14s %-10s %-10s %-10s\n",
           "Tipo", "Disponiveis", "Ocupadas", "Taxa");
    print_separator('-', 50);

    for (int t = 0; t < 4; t++) {
        double pct = total_nights[t] > 0
                     ? 100.0 * occ_nights[t] / total_nights[t] : 0.0;
        printf("  %-14s %-10ld %-10ld %.1f%%\n",
               type_names[t], total_nights[t], occ_nights[t], pct);
        total_occ   += occ_nights[t];
        total_avail += total_nights[t];
    }

    print_separator('-', 50);
    double overall = total_avail > 0 ? 100.0 * total_occ / total_avail : 0.0;
    printf("  %-14s %-10ld %-10ld %.1f%%\n",
           "TOTAL", total_avail, total_occ, overall);

    /* RevPAR e ADR */
    double revenue = 0.0;
    int    checked = 0;
    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status != RES_CHECKOUT_FEITO) continue;
        time_t rs = r->check_in_date  > t_from ? r->check_in_date  : t_from;
        time_t re = r->check_out_date < t_to   ? r->check_out_date : t_to;
        if (date_diff_days(rs, re) <= 0) continue;
        revenue += r->total_value;
        checked++;
    }
    if (total_avail > 0)
        printf("\n  RevPAR : R$ %.2f / noite disponivel\n",
               revenue / total_avail);
    if (checked > 0)
        printf("  ADR    : R$ %.2f / estadia\n", revenue / checked);

    pause_screen();
}

/* =========================================================
 *  Ranking de hóspedes
 * ========================================================= */
void report_top_guests(void) {
    if (!auth_require(ACCESS_BASICO)) return;

    clear_screen();
    print_header("RANKING DE HOSPEDES");

    printf("  Ordenar por: [1] Numero de estadias   [2] Valor gasto\n");
    int by = input_int("  Opcao", 1, 2);
    int top = input_int("  Quantos listar (top N)", 1, 50);

    /* Ordena cópia dos índices */
    int idx[MAX_GUESTS];
    for (int i = 0; i < g_guest_count; i++) idx[i] = i;

    /* Insertion sort — listas pequenas */
    for (int i = 1; i < g_guest_count; i++) {
        int key = idx[i], j = i - 1;
        while (j >= 0) {
            int better = (by == 1)
                ? (g_guests[idx[j]].total_stays  < g_guests[key].total_stays)
                : (g_guests[idx[j]].total_spent < g_guests[key].total_spent);
            if (!better) break;
            idx[j + 1] = idx[j];
            j--;
        }
        idx[j + 1] = key;
    }

    printf("\n  %-4s %-30s %-12s %-10s %-10s %s\n",
           "#", "Nome", "Documento", "Estadias", "Gasto (R$)", "Fidelidade");
    print_separator('-', 75);

    int shown = 0;
    for (int k = 0; k < g_guest_count && shown < top; k++) {
        Guest *g = &g_guests[idx[k]];
        printf("  %-4d %-30s %-12s %-10d R$%-8.2f %s\n",
               shown + 1, g->full_name, g->document,
               g->total_stays, g->total_spent,
               guest_fidelity_str(g->fidelity_level));
        shown++;
    }
    print_separator('-', 75);
    pause_screen();
}

/* =========================================================
 *  No-shows e cancelamentos
 * ========================================================= */
void report_noshow_cancel(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("NO-SHOWS E CANCELAMENTOS");

    char from_str[12], to_str[12];
    do {
        input_string("  Data inicial (DD/MM/AAAA): ", from_str, sizeof(from_str));
        if (!date_is_valid(from_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(from_str));
    do {
        input_string("  Data final   (DD/MM/AAAA): ", to_str, sizeof(to_str));
        if (!date_is_valid(to_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(to_str));

    time_t t_from = date_from_string(from_str);
    time_t t_to   = date_from_string(to_str) + 86400;

    int ns = 0, can = 0;
    double loss = 0.0;

    printf("\n  %-5s %-12s %-25s %-14s %s\n",
           "ID", "Codigo", "Hospede", "Status", "Perda (R$)");
    print_separator('-', 68);

    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status != RES_CANCELADA && r->status != RES_NO_SHOW) continue;
        if (r->check_in_date < t_from || r->check_in_date >= t_to) continue;

        Guest *g = guest_find_by_id(r->guest_id);
        double pen = (r->status == RES_NO_SHOW) ? r->base_value
                   : (r->base_value - r->discount);

        printf("  %-5d %-12s %-25s %-14s R$ %.2f\n",
               r->id, r->code,
               g ? g->full_name : "?",
               res_status_str(r->status),
               pen);
        loss += pen;
        if (r->status == RES_NO_SHOW) ns++;
        else                           can++;
    }
    print_separator('-', 68);
    printf("  No-shows: %d  | Cancelamentos: %d  | Perda total: R$ %.2f\n",
           ns, can, loss);
    pause_screen();
}

/* =========================================================
 *  Previsão de ocupação — próximos 30 dias
 * ========================================================= */
void report_forecast(void) {
    if (!auth_require(ACCESS_BASICO)) return;

    clear_screen();
    print_header("PREVISAO DE OCUPACAO — PROXIMOS 30 DIAS");

    time_t today = date_today();

    printf("  %-12s %-10s %-10s %-6s\n",
           "Data", "Quartos", "Ocup.", "Taxa");
    print_separator('-', 42);

    for (int d = 0; d < 30; d++) {
        time_t day_start = today + (time_t)d * 86400;
        time_t day_end   = day_start + 86400;
        char   dt[12];
        date_to_string(day_start, dt);

        int occ = 0;
        for (int i = 0; i < g_reservation_count; i++) {
            Reservation *r = &g_reservations[i];
            if (r->status == RES_CANCELADA || r->status == RES_NO_SHOW
                || r->status == RES_CHECKOUT_FEITO) continue;
            if (r->check_in_date < day_end && r->check_out_date > day_start)
                occ += r->room_count;
        }

        int avail = g_room_count;
        double pct = avail > 0 ? 100.0 * occ / avail : 0.0;
        /* Barra visual */
        int bar = (int)(pct / 5.0);
        char vis[22];
        int k;
        for (k = 0; k < bar && k < 20; k++) vis[k] = '#';
        for (; k < 20; k++) vis[k] = '.';
        vis[20] = '\0';

        printf("  %-12s %-10d %-10d %5.1f%% [%s]\n",
               dt, avail, occ, pct, vis);
    }
    pause_screen();
}

/* =========================================================
 *  Exportar receita do período para CSV
 * ========================================================= */
void report_export_csv(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("EXPORTAR RELATORIO CSV");

    char from_str[12], to_str[12];
    do {
        input_string("  Data inicial (DD/MM/AAAA): ", from_str, sizeof(from_str));
        if (!date_is_valid(from_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(from_str));
    do {
        input_string("  Data final   (DD/MM/AAAA): ", to_str, sizeof(to_str));
        if (!date_is_valid(to_str)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(to_str));

    time_t t_from = date_from_string(from_str);
    time_t t_to   = date_from_string(to_str) + 86400;

    /* Monta nome do arquivo: reports/receita_DDMMAAAA_DDMMAAAA.csv */
    char fname[80];
    char f2[9], t2[9];
    /* Remove '/' para montar nome seguro */
    {
        int fi = 0, ti2 = 0;
        for (int c = 0; from_str[c]; c++) if (from_str[c] != '/') f2[fi++] = from_str[c];
        f2[fi] = '\0';
        for (int c = 0; to_str[c];   c++) if (to_str[c]   != '/') t2[ti2++] = to_str[c];
        t2[ti2] = '\0';
    }
    snprintf(fname, sizeof(fname), "reports/receita_%s_%s.csv", f2, t2);

    FILE *fp = fopen(fname, "w");
    if (!fp) {
        perror("  [!] Erro ao criar arquivo");
        pause_screen();
        return;
    }

    fprintf(fp, "ID,Codigo,Hospede,Documento,CheckIn,CheckOut,Diarias,Servicos,Desconto,Total,Pagamento\n");

    int count = 0;
    for (int i = 0; i < g_reservation_count; i++) {
        Reservation *r = &g_reservations[i];
        if (r->status != RES_CHECKOUT_FEITO) continue;
        if (r->actual_check_out < t_from || r->actual_check_out >= t_to) continue;

        Guest *g = guest_find_by_id(r->guest_id);
        char ci[14], co[14];
        date_to_string(r->check_in_date,  ci);
        date_to_string(r->check_out_date, co);

        fprintf(fp, "%d,%s,\"%s\",%s,%s,%s,%.2f,%.2f,%.2f,%.2f,%s\n",
                r->id, r->code,
                g ? g->full_name : "",
                g ? g->document  : "",
                ci, co,
                r->base_value, r->services_total,
                r->discount, r->total_value,
                pay_method_str(r->payment_method));
        count++;
    }
    fclose(fp);

    char log[128];
    snprintf(log, sizeof(log), "EXPORT CSV %s count=%d", fname, count);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] %d registro(s) exportado(s) para:\n  %s\n", count, fname);
    pause_screen();
}

/* =========================================================
 *  Helpers locais para CSV
 * ========================================================= */
static const char *clean_status_str_local(CleanStatus cs) {
    switch (cs) {
        case CLEAN_SUJO:         return "Sujo";
        case CLEAN_EM_LIMPEZA:   return "Em Limpeza";
        case CLEAN_LIMPO:        return "Limpo";
        case CLEAN_INSPECIONADO: return "Inspecionado";
        default:                 return "?";
    }
}

/* =========================================================
 *  Exportar quartos para CSV
 * ========================================================= */
void report_export_rooms_csv(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("EXPORTAR QUARTOS CSV");

    time_t     now = time(NULL);
    struct tm *lt  = localtime(&now);
    char fname[80];
    strftime(fname, sizeof(fname), "reports/quartos_%d%m%Y_%H%M%S.csv", lt);

    FILE *fp = fopen(fname, "w");
    if (!fp) { perror("  [!] Erro ao criar arquivo"); pause_screen(); return; }

    fprintf(fp, "ID,Numero,Andar,Tipo,Capacidade,Diaria,Status,Limpeza,Caracteristicas\n");

    int count = 0;
    for (int i = 0; i < g_room_count; i++) {
        Room *r = &g_rooms[i];
        char feats[60] = "";
        if (r->features & FEAT_AC)        strcat(feats, "AC ");
        if (r->features & FEAT_FRIGOBAR)  strcat(feats, "Frigobar ");
        if (r->features & FEAT_BANHEIRA)  strcat(feats, "Banheira ");
        if (r->features & FEAT_VARANDA)   strcat(feats, "Varanda ");
        if (r->features & FEAT_ACESSIVEL) strcat(feats, "Acessivel");
        fprintf(fp, "%d,%s,%d,%s,%d,%.2f,%s,%s,\"%s\"\n",
                r->id, r->number, r->floor,
                room_type_str(r->type), r->capacity, r->daily_rate,
                room_status_str(r->status),
                clean_status_str_local(r->clean_status),
                feats);
        count++;
    }
    fclose(fp);

    char log[128];
    snprintf(log, sizeof(log), "EXPORT ROOMS CSV %s count=%d", fname, count);
    storage_audit_log(g_session.staff.id, log);
    printf("\n  [OK] %d quarto(s) exportado(s) para:\n  %s\n", count, fname);
    pause_screen();
}

/* =========================================================
 *  Exportar hóspedes para CSV
 * ========================================================= */
void report_export_guests_csv(void) {
    if (!auth_require(ACCESS_SUPERVISOR)) return;

    clear_screen();
    print_header("EXPORTAR HOSPEDES CSV");

    time_t     now = time(NULL);
    struct tm *lt  = localtime(&now);
    char fname[80];
    strftime(fname, sizeof(fname), "reports/hospedes_%d%m%Y_%H%M%S.csv", lt);

    FILE *fp = fopen(fname, "w");
    if (!fp) { perror("  [!] Erro ao criar arquivo"); pause_screen(); return; }

    fprintf(fp, "ID,Nome,Documento,Nascimento,Nacionalidade,Telefone,Email,"
                "Tipo,Fidelidade,Pontos,Estadas,TotalGasto,UltimaVisita\n");

    static const char *gtypes[] = {"Regular", "VIP", "Corporativo", "Grupo"};
    int count = 0;
    for (int i = 0; i < g_guest_count; i++) {
        Guest *g2 = &g_guests[i];
        char last_visit[20] = "Nunca";
        if (g2->last_visit > 0) datetime_to_string(g2->last_visit, last_visit);
        const char *gtype = ((int)g2->type >= 0 && (int)g2->type <= 3)
                            ? gtypes[(int)g2->type] : "?";
        fprintf(fp, "%d,\"%s\",%s,%s,%s,%s,\"%s\",%s,%s,%d,%d,%.2f,%s\n",
                g2->id, g2->full_name, g2->document, g2->birth_date,
                g2->nationality, g2->phone, g2->email,
                gtype,
                guest_fidelity_str(g2->fidelity_level),
                g2->fidelity_points, g2->total_stays, g2->total_spent,
                last_visit);
        count++;
    }
    fclose(fp);

    char log[128];
    snprintf(log, sizeof(log), "EXPORT GUESTS CSV %s count=%d", fname, count);
    storage_audit_log(g_session.staff.id, log);
    printf("\n  [OK] %d hospede(s) exportado(s) para:\n  %s\n", count, fname);
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static const MenuItem report_items[] = {
    {1, "Taxa de ocupacao",            ACCESS_BASICO,    report_occupation},
    {2, "Ranking de hospedes",         ACCESS_BASICO,    report_top_guests},
    {3, "No-shows e cancelamentos",    ACCESS_SUPERVISOR,report_noshow_cancel},
    {4, "Previsao 30 dias",            ACCESS_BASICO,    report_forecast},
    {5, "Exportar CSV receita",        ACCESS_SUPERVISOR,report_export_csv},
    {6, "Exportar CSV quartos",        ACCESS_SUPERVISOR,report_export_rooms_csv},
    {7, "Exportar CSV hospedes",       ACCESS_SUPERVISOR,report_export_guests_csv},
    {0, "Voltar",                      0,                NULL}
};

void report_menu(void) {
    menu_run("MODULO: RELATORIOS",
             report_items,
             (int)(sizeof(report_items) / sizeof(report_items[0])));
}
