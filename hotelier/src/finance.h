#ifndef FINANCE_H
#define FINANCE_H

#include "types.h"

/* ── Helpers ──────────────────────────────────────────── */
const char *pay_method_str(PaymentMethod m);

/* Consolida base + serviços + taxas e atualiza r->total_value */
void   finance_calc_total(Reservation *r);

/* ── Faturamento ─────────────────────────────────────── */
void   finance_invoice(void);   /* cobra uma reserva (checkout ou avulso) */

/* ── Caixa ──────────────────────────────────────────── */
void   finance_daily_report(void);    /* receita do dia */
void   finance_period_report(void);   /* receita por período */
void   finance_receivables(void);     /* contas a receber (PAY_FATURADO) */

/* ── Ponto de entrada do módulo ───────────────────────── */
void   finance_menu(void);

#endif /* FINANCE_H */
