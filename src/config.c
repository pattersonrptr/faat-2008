#include "config.h"
#include "storage.h"
#include "auth.h"
#include "utils.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>

/* =========================================================
 *  Exibir configuracao atual
 * ========================================================= */
static void config_view(void) {
    clear_screen();
    print_header("CONFIGURACOES DO HOTEL");

    printf("  -- Dados do Hotel --\n");
    printf("  Nome             : %s\n", g_config.hotel_name);
    printf("  Endereco         : %s\n", g_config.address);
    printf("  CNPJ             : %s\n", g_config.cnpj);
    printf("  Categoria        : %d estrela(s)\n", g_config.stars);
    printf("  Horario Check-in : %s\n", g_config.checkin_time);
    printf("  Horario Check-out: %s\n", g_config.checkout_time);
    printf("\n  -- Taxas e Politicas --\n");
    printf("  Taxa turistica (R$/noite)  : R$ %.2f\n", g_config.tourist_tax);
    printf("  Taxa de servico            : %.0f%%\n",  g_config.service_fee_pct * 100.0);
    printf("  Taxa late check-out        : R$ %.2f\n", g_config.late_checkout_fee);
    printf("  Taxa early check-in        : R$ %.2f\n", g_config.early_checkin_fee);
    printf("  Cancelamento gratis (dias) : %d\n",      g_config.cancel_free_days);
    printf("  Multa cancelamento parcial : %.0f%%\n",  g_config.cancel_partial_pct * 100.0);
    printf("\n  -- Programa de Fidelidade --\n");
    printf("  Pontos por R$1,00 gasto    : %.2f\n",    g_config.points_per_real);
    printf("  Multiplicador Prata        : %.1fx\n",   g_config.multiplier_prata);
    printf("  Multiplicador Ouro         : %.1fx\n",   g_config.multiplier_ouro);
    printf("  Multiplicador Diamante     : %.1fx\n",   g_config.multiplier_diamante);

    pause_screen();
}

/* =========================================================
 *  Editar dados do hotel
 * ========================================================= */
static void config_edit_hotel(void) {
    if (!auth_require(ACCESS_ADMIN)) return;

    clear_screen();
    print_header("DADOS DO HOTEL");

    printf("  Nome atual      : %s\n", g_config.hotel_name);
    input_string("  Novo nome (Enter = manter): ", g_config.hotel_name, sizeof(g_config.hotel_name));

    printf("  Endereco atual  : %s\n", g_config.address);
    input_string("  Novo endereco  : ", g_config.address, sizeof(g_config.address));

    printf("  CNPJ atual      : %s\n", g_config.cnpj);
    input_string("  Novo CNPJ      : ", g_config.cnpj, sizeof(g_config.cnpj));

    g_config.stars = input_int("  Categoria (estrelas)", 1, 5);

    printf("  Horario check-in  atual: %s\n", g_config.checkin_time);
    input_string("  Novo horario check-in  (HH:MM): ", g_config.checkin_time, sizeof(g_config.checkin_time));

    printf("  Horario check-out atual: %s\n", g_config.checkout_time);
    input_string("  Novo horario check-out (HH:MM): ", g_config.checkout_time, sizeof(g_config.checkout_time));

    storage_save_config();
    storage_audit_log(g_session.staff.id, "CONFIG HOTEL UPDATED");
    printf("\n  [OK] Dados do hotel atualizados.\n");
    pause_screen();
}

/* =========================================================
 *  Editar taxas e politicas financeiras
 * ========================================================= */
static void config_edit_financial(void) {
    if (!auth_require(ACCESS_ADMIN)) return;

    clear_screen();
    print_header("TAXAS E POLITICAS");

    g_config.tourist_tax = input_double("  Taxa turistica por noite (R$)", 0.0, 9999.0);

    double pct = input_double("  Taxa de servico (%)", 0.0, 100.0);
    g_config.service_fee_pct = pct / 100.0;

    g_config.late_checkout_fee  = input_double("  Taxa late check-out (R$)",  0.0, 9999.0);
    g_config.early_checkin_fee  = input_double("  Taxa early check-in (R$)",  0.0, 9999.0);
    g_config.cancel_free_days   = input_int(   "  Dias para cancelamento gratis", 0, 365);

    double cpct = input_double("  Multa de cancelamento parcial (%)", 0.0, 100.0);
    g_config.cancel_partial_pct = cpct / 100.0;

    storage_save_config();
    storage_audit_log(g_session.staff.id, "CONFIG FINANCIAL UPDATED");
    printf("\n  [OK] Taxas e politicas atualizadas.\n");
    pause_screen();
}

/* =========================================================
 *  Editar programa de fidelidade
 * ========================================================= */
static void config_edit_fidelity(void) {
    if (!auth_require(ACCESS_ADMIN)) return;

    clear_screen();
    print_header("PROGRAMA DE FIDELIDADE");

    g_config.points_per_real     = input_double("  Pontos por R$1,00 gasto",     0.01, 100.0);
    g_config.multiplier_prata    = input_double("  Multiplicador Prata  (ex: 1.5)", 1.0, 10.0);
    g_config.multiplier_ouro     = input_double("  Multiplicador Ouro   (ex: 2.0)", 1.0, 10.0);
    g_config.multiplier_diamante = input_double("  Multiplicador Diamante(ex: 3.0)", 1.0, 10.0);

    storage_save_config();
    storage_audit_log(g_session.staff.id, "CONFIG FIDELITY UPDATED");
    printf("\n  [OK] Programa de fidelidade atualizado.\n");
    pause_screen();
}

/* =========================================================
 *  Alterar senha do operador logado
 * ========================================================= */
static void config_change_password(void) {
    clear_screen();
    print_header("ALTERAR SENHA");

    /* Verifica senha atual */
    char old_pw[64];
    input_string("  Senha atual     : ", old_pw, sizeof(old_pw));

    unsigned long h_old = hash_djb2(old_pw);
    char hex_old[20];
    hash_to_hex(h_old, hex_old);

    if (strcmp(hex_old, g_session.staff.password_hash) != 0) {
        printf("  [!] Senha atual incorreta.\n");
        pause_screen();
        return;
    }

    char new_pw[64], confirm_pw[64];
    input_string("  Nova senha       : ", new_pw,     sizeof(new_pw));
    input_string("  Confirme a senha : ", confirm_pw, sizeof(confirm_pw));

    if (strcmp(new_pw, confirm_pw) != 0) {
        printf("  [!] As senhas nao conferem.\n");
        pause_screen();
        return;
    }
    if (strlen(new_pw) < 6) {
        printf("  [!] A senha deve ter pelo menos 6 caracteres.\n");
        pause_screen();
        return;
    }

    /* Atualiza no array de staff */
    for (int i = 0; i < g_staff_count; i++) {
        if (g_staff[i].id == g_session.staff.id) {
            unsigned long h_new = hash_djb2(new_pw);
            hash_to_hex(h_new, g_staff[i].password_hash);
            /* Atualiza tambem na sessao ativa */
            hash_to_hex(h_new, g_session.staff.password_hash);
            break;
        }
    }

    storage_save_staff();
    storage_audit_log(g_session.staff.id, "PASSWORD CHANGED");
    printf("\n  [OK] Senha alterada com sucesso.\n");
    pause_screen();
}

/* =========================================================
 *  Menu
 * ========================================================= */
static const MenuItem config_items[] = {
    {1, "Ver configuracoes",       ACCESS_BASICO,   config_view},
    {2, "Dados do hotel",          ACCESS_ADMIN,    config_edit_hotel},
    {3, "Taxas e politicas",       ACCESS_ADMIN,    config_edit_financial},
    {4, "Programa de fidelidade",  ACCESS_ADMIN,    config_edit_fidelity},
    {5, "Alterar minha senha",     ACCESS_BASICO,   config_change_password},
    {0, "Voltar",                  0,               NULL}
};

void config_menu(void) {
    menu_run("MODULO: CONFIGURACOES",
             config_items,
             (int)(sizeof(config_items) / sizeof(config_items[0])));
}
