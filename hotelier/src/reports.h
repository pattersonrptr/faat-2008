#ifndef REPORTS_H
#define REPORTS_H

/* ── Relatórios ───────────────────────────────────────── */
void report_occupation(void);        /* taxa de ocupação por período          */
void report_top_guests(void);        /* ranking de hóspedes (estadias / gasto)*/
void report_noshow_cancel(void);     /* no-shows e cancelamentos              */
void report_forecast(void);          /* previsão de ocupação 30 dias          */
void report_export_csv(void);        /* exporta receita do período para .csv  */

/* ── Ponto de entrada do módulo ───────────────────────── */
void report_menu(void);

#endif /* REPORTS_H */
