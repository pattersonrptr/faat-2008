#include "guests.h"
#include "storage.h"
#include "auth.h"
#include "utils.h"
#include "menu.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* =========================================================
 *  Helpers de conversão
 * ========================================================= */
const char *guest_type_str(GuestType t) {
    switch (t) {
        case GUEST_REGULAR:     return "Regular";
        case GUEST_VIP:         return "VIP";
        case GUEST_CORPORATIVO: return "Corporativo";
        case GUEST_GRUPO:       return "Grupo";
        default:                return "?";
    }
}

const char *guest_fidelity_str(FidelityLevel l) {
    switch (l) {
        case FIDELITY_BRONZE:   return "Bronze";
        case FIDELITY_PRATA:    return "Prata";
        case FIDELITY_OURO:     return "Ouro";
        case FIDELITY_DIAMANTE: return "Diamante";
        default:                return "?";
    }
}

/* =========================================================
 *  Busca
 * ========================================================= */
Guest *guest_find_by_id(int id) {
    for (int i = 0; i < g_guest_count; i++) {
        if (g_guests[i].id == id) return &g_guests[i];
    }
    return NULL;
}

Guest *guest_find_by_document(const char *doc) {
    for (int i = 0; i < g_guest_count; i++) {
        if (strcmp(g_guests[i].document, doc) == 0) return &g_guests[i];
    }
    return NULL;
}

/* =========================================================
 *  Exibição
 * ========================================================= */
static void print_guest_table_header(void) {
    printf("  %-5s %-30s %-16s %-12s %-10s %s\n",
           "ID", "Nome", "Documento", "Tipo", "Fidelidade", "BL");
    print_separator('-', 80);
}

void guest_print_brief(const Guest *g) {
    printf("  %-5d %-30s %-16s %-12s %-10s %s\n",
           g->id,
           g->full_name,
           g->document,
           guest_type_str(g->type),
           guest_fidelity_str(g->fidelity_level),
           g->blacklisted ? "[!]" : "");
}

void guest_print_full(const Guest *g) {
    char dt[20];
    print_separator('-', 60);
    printf("  Hospede #%d\n", g->id);
    print_separator('-', 60);
    printf("  Nome        : %s\n",  g->full_name);
    printf("  Documento   : %s\n",  g->document);
    printf("  Nascimento  : %s\n",  g->birth_date);
    printf("  Sexo        : %c\n",  g->gender);
    printf("  Nac.        : %s\n",  g->nationality);
    printf("  Telefone    : %s\n",  g->phone);
    printf("  E-mail      : %s\n",  g->email);
    printf("  Endereco    : %s\n",  g->address);
    printf("  Tipo        : %s\n",  guest_type_str(g->type));
    printf("  Fidelidade  : %s  (%d pontos)\n",
           guest_fidelity_str(g->fidelity_level), g->fidelity_points);
    printf("  Estadias    : %d   |  Total gasto: R$ %.2f\n",
           g->total_stays, g->total_spent);
    if (g->last_visit > 0) {
        datetime_to_string(g->last_visit, dt);
        printf("  Ultima visit: %s\n", dt);
    }
    if (g->blacklisted) {
        printf("\n  ** BLACKLIST ** : %s\n", g->blacklist_reason);
    }
    print_separator('-', 60);
}

void guest_list(void) {
    clear_screen();
    print_header("LISTA DE HOSPEDES");

    if (g_guest_count == 0) {
        printf("  Nenhum hospede cadastrado.\n");
        pause_screen();
        return;
    }

    print_guest_table_header();
    for (int i = 0; i < g_guest_count; i++) {
        guest_print_brief(&g_guests[i]);
    }
    print_separator('-', 80);
    printf("  Total: %d hospede(s)\n", g_guest_count);
    pause_screen();
}

void guest_search(void) {
    clear_screen();
    print_header("BUSCAR HOSPEDE");
    printf("  [1] Por nome (parcial)   [2] Por documento   [0] Cancelar\n");
    int opt = input_int("  Opcao", 0, 2);
    if (opt == 0) return;

    char term[100];
    if (opt == 1) input_string("  Nome     : ", term, sizeof(term));
    else          input_string("  Documento: ", term, sizeof(term));

    int found = 0;
    printf("\n");
    print_guest_table_header();
    for (int i = 0; i < g_guest_count; i++) {
        int match = (opt == 1)
            ? str_contains(g_guests[i].full_name, term)
            : str_contains(g_guests[i].document, term);
        if (match) {
            guest_print_brief(&g_guests[i]);
            found++;
        }
    }
    print_separator('-', 80);
    printf("  %d resultado(s) encontrado(s).\n", found);

    if (found > 0 && input_confirm("\n  Ver ficha completa de um hospede")) {
        int id = input_int("  ID do hospede", 1, 999999);
        Guest *g = guest_find_by_id(id);
        if (g) guest_print_full(g);
        else   printf("  ID nao encontrado.\n");
        pause_screen();
    } else {
        pause_screen();
    }
}

/* =========================================================
 *  Helpers internos
 * ========================================================= */
static FidelityLevel calc_fidelity_level(int points) {
    if (points >= 10000) return FIDELITY_DIAMANTE;
    if (points >= 5000)  return FIDELITY_OURO;
    if (points >= 1000)  return FIDELITY_PRATA;
    return FIDELITY_BRONZE;
}

/* =========================================================
 *  CRUD e operações
 * ========================================================= */
void guest_add(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("CADASTRAR HOSPEDE");

    if (g_guest_count >= MAX_GUESTS) {
        printf("  [!] Limite maximo de hospedes atingido.\n");
        pause_screen();
        return;
    }

    Guest g;
    memset(&g, 0, sizeof(g));

    input_string("  Nome completo      : ", g.full_name, sizeof(g.full_name));

    printf("  Tipo de documento: [1] CPF   [2] Passaporte\n");
    int doc_type = input_int("  Tipo", 1, 2);

    if (doc_type == 1) {
        char raw[20];
        do {
            input_string("  CPF (somente numeros): ", raw, sizeof(raw));
            if (!cpf_is_valid(raw)) printf("  [!] CPF invalido. Tente novamente.\n");
        } while (!cpf_is_valid(raw));
        cpf_format(raw, g.document);
    } else {
        input_string("  Passaporte: ", g.document, sizeof(g.document));
        str_upper(g.document);
    }

    if (guest_find_by_document(g.document) != NULL) {
        printf("  [!] Ja existe um hospede com este documento.\n");
        pause_screen();
        return;
    }

    do {
        input_string("  Data de nascimento (DD/MM/AAAA): ", g.birth_date, sizeof(g.birth_date));
        if (!date_is_valid(g.birth_date)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(g.birth_date));

    char sexbuf[4];
    printf("  Sexo: [M] Masculino   [F] Feminino   [O] Outro\n");
    do {
        input_string("  Sexo: ", sexbuf, sizeof(sexbuf));
        str_upper(sexbuf);
    } while (sexbuf[0] != 'M' && sexbuf[0] != 'F' && sexbuf[0] != 'O');
    g.gender = sexbuf[0];

    input_string("  Nacionalidade     : ", g.nationality, sizeof(g.nationality));
    input_string("  Telefone          : ", g.phone,       sizeof(g.phone));
    input_string("  E-mail            : ", g.email,       sizeof(g.email));
    input_string("  Endereco          : ", g.address,     sizeof(g.address));

    printf("  Tipo: [0] Regular   [1] VIP   [2] Corporativo   [3] Grupo\n");
    g.type = (GuestType)input_int("  Tipo", 0, 3);

    g.id             = storage_next_guest_id();
    g.fidelity_level = FIDELITY_BRONZE;

    g_guests[g_guest_count++] = g;
    storage_save_guests();

    char log[80];
    snprintf(log, sizeof(log), "HOSPEDE ADD id=%d doc=%s", g.id, g.document);
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Hospede cadastrado com sucesso (ID: %d).\n", g.id);
    pause_screen();
}

void guest_edit(void) {
    if (!auth_require(ACCESS_OPERADOR)) return;

    clear_screen();
    print_header("EDITAR HOSPEDE");

    int id = input_int("  ID do hospede", 1, 999999);
    Guest *g = guest_find_by_id(id);
    if (g == NULL) {
        printf("  [!] Hospede ID %d nao encontrado.\n", id);
        pause_screen();
        return;
    }

    guest_print_full(g);
    printf("  O que deseja alterar?\n");
    printf("  [1] Nome   [2] Telefone   [3] E-mail   [4] Endereco   [5] Tipo   [0] Cancelar\n");
    int opt = input_int("  Opcao", 0, 5);

    switch (opt) {
        case 1: input_string("  Novo nome     : ", g->full_name, sizeof(g->full_name)); break;
        case 2: input_string("  Novo telefone : ", g->phone,     sizeof(g->phone));     break;
        case 3: input_string("  Novo e-mail   : ", g->email,     sizeof(g->email));     break;
        case 4: input_string("  Novo endereco : ", g->address,   sizeof(g->address));   break;
        case 5:
            printf("  [0] Regular  [1] VIP  [2] Corporativo  [3] Grupo\n");
            g->type = (GuestType)input_int("  Novo tipo", 0, 3);
            break;
        case 0:
            return;
    }

    storage_save_guests();
    char log[80];
    snprintf(log, sizeof(log), "HOSPEDE EDIT id=%d campo=%d", g->id, opt);
    storage_audit_log(g_session.staff.id, log);
    printf("\n  [OK] Hospede atualizado.\n");
    pause_screen();
}

void guest_toggle_blacklist(void) {
    if (!auth_require(ACCESS_GERENTE)) return;

    clear_screen();
    print_header("GERENCIAR BLACKLIST");

    int id = input_int("  ID do hospede", 1, 999999);
    Guest *g = guest_find_by_id(id);
    if (g == NULL) {
        printf("  [!] Hospede ID %d nao encontrado.\n", id);
        pause_screen();
        return;
    }

    printf("  Hospede: %s\n", g->full_name);
    printf("  Status atual: %s\n\n", g->blacklisted ? "** NA BLACKLIST **" : "Normal");

    if (g->blacklisted) {
        if (input_confirm("  Remover da blacklist")) {
            g->blacklisted = 0;
            g->blacklist_reason[0] = '\0';
            printf("\n  [OK] Hospede removido da blacklist.\n");
        }
    } else {
        if (input_confirm("  Incluir na blacklist")) {
            input_string("  Motivo: ", g->blacklist_reason, sizeof(g->blacklist_reason));
            g->blacklisted = 1;
            printf("\n  [OK] Hospede incluido na blacklist.\n");
        }
    }

    storage_save_guests();
    char log[80];
    snprintf(log, sizeof(log), "HOSPEDE BLACKLIST id=%d status=%d", g->id, g->blacklisted);
    storage_audit_log(g_session.staff.id, log);
    pause_screen();
}

void guest_update_fidelity(int guest_id, double amount_spent) {
    Guest *g = guest_find_by_id(guest_id);
    if (g == NULL) return;

    double multiplier = 1.0;
    switch (g->fidelity_level) {
        case FIDELITY_PRATA:    multiplier = g_config.multiplier_prata;    break;
        case FIDELITY_OURO:     multiplier = g_config.multiplier_ouro;     break;
        case FIDELITY_DIAMANTE: multiplier = g_config.multiplier_diamante; break;
        default: break;
    }

    int pts = (int)(amount_spent * g_config.points_per_real * multiplier);
    g->fidelity_points += pts;
    g->total_spent     += amount_spent;
    g->total_stays++;
    g->last_visit = time(NULL);

    FidelityLevel old = g->fidelity_level;
    g->fidelity_level = calc_fidelity_level(g->fidelity_points);
    if (g->fidelity_level != old) {
        printf("  [*] Hospede promovido para nivel %s!\n",
               guest_fidelity_str(g->fidelity_level));
    }

    storage_save_guests();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static const MenuItem guest_items[] = {
    {1, "Listar hospedes",        ACCESS_BASICO,    guest_list},
    {2, "Buscar hospede",         ACCESS_BASICO,    guest_search},
    {3, "Cadastrar hospede",      ACCESS_OPERADOR,  guest_add},
    {4, "Editar hospede",         ACCESS_OPERADOR,  guest_edit},
    {5, "Gerenciar blacklist",    ACCESS_GERENTE,   guest_toggle_blacklist},
    {0, "Voltar",                 0,                NULL}
};

void guest_menu(void) {
    menu_run("MODULO: HOSPEDES",
             guest_items,
             (int)(sizeof(guest_items) / sizeof(guest_items[0])));
}
