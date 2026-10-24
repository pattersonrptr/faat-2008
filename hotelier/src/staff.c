#include "staff.h"
#include "storage.h"
#include "auth.h"
#include "utils.h"
#include "menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
 *  Helpers
 * ========================================================= */
const char *staff_role_str(StaffRole r) {
    switch (r) {
        case ROLE_RECEPCIONISTA: return "Recepcionista";
        case ROLE_GERENTE:       return "Gerente";
        case ROLE_CAMAREIRO:     return "Camareiro";
        case ROLE_MANUTENCAO:    return "Manutencao";
        case ROLE_SEGURANCA:     return "Seguranca";
        case ROLE_COZINHEIRO:    return "Cozinheiro";
        case ROLE_GARCOM:        return "Garcom";
        default:                 return "?";
    }
}

const char *staff_shift_str(int shift) {
    switch (shift) {
        case 0:  return "Manha";
        case 1:  return "Tarde";
        case 2:  return "Noite";
        default: return "?";
    }
}

/* =========================================================
 *  Busca
 * ========================================================= */
Staff *staff_find_by_id(int id) {
    for (int i = 0; i < g_staff_count; i++) {
        if (g_staff[i].id == id) return &g_staff[i];
    }
    return NULL;
}

Staff *staff_find_by_username(const char *username) {
    for (int i = 0; i < g_staff_count; i++) {
        if (strcmp(g_staff[i].username, username) == 0) return &g_staff[i];
    }
    return NULL;
}

/* =========================================================
 *  Exibição
 * ========================================================= */
void staff_print_brief(const Staff *s) {
    printf("  %-4d %-25s %-16s %-14s Nv.%-2d Turno:%-5s %s\n",
           s->id, s->full_name, s->document,
           staff_role_str(s->role),
           s->access_level,
           staff_shift_str(s->shift),
           s->active ? "ATIVO" : "[INATIVO]");
}

void staff_print_full(const Staff *s) {
    print_separator('-', 60);
    printf("  Funcionario #%d\n", s->id);
    print_separator('-', 60);
    printf("  Nome        : %s\n", s->full_name);
    printf("  Documento   : %s\n", s->document);
    printf("  Cargo       : %s\n", staff_role_str(s->role));
    printf("  Depto       : %s\n", s->department);
    printf("  Admissao    : %s\n", s->hire_date);
    printf("  Salario     : R$ %.2f\n", s->salary);
    printf("  Turno       : %s\n", staff_shift_str(s->shift));
    printf("  Nivel acesso: %d\n", s->access_level);
    printf("  Usuario     : %s\n", s->username);
    printf("  Status      : %s\n", s->active ? "ATIVO" : "INATIVO");
    print_separator('-', 60);
}

void staff_list(void) {
    clear_screen();
    print_header("LISTA DE FUNCIONARIOS");

    printf("  %-4s %-25s %-16s %-14s %-4s %-7s %s\n",
           "ID","Nome","Documento","Cargo","Nv.","Turno","Status");
    print_separator('-', 78);

    for (int i = 0; i < g_staff_count; i++) {
        staff_print_brief(&g_staff[i]);
    }
    print_separator('-', 78);
    printf("  Total: %d funcionario(s)\n", g_staff_count);
    pause_screen();
}

/* =========================================================
 *  CRUD e operações
 * ========================================================= */
void staff_add(void) {
    if (!auth_require(ACCESS_GERENTE)) return;

    clear_screen();
    print_header("CADASTRAR FUNCIONARIO");

    if (g_staff_count >= MAX_STAFF) {
        printf("  [!] Limite maximo de funcionarios atingido (%d).\n", MAX_STAFF);
        pause_screen();
        return;
    }

    Staff s;
    memset(&s, 0, sizeof(s));

    input_string("  Nome completo      : ", s.full_name,   sizeof(s.full_name));
    input_string("  CPF/documento      : ", s.document,    sizeof(s.document));
    input_string("  Departamento       : ", s.department,  sizeof(s.department));

    printf("  Cargo:\n");
    printf("  [0]Recepcionista [1]Gerente [2]Camareiro [3]Manutencao\n");
    printf("  [4]Seguranca     [5]Cozinheiro [6]Garcom\n");
    s.role = (StaffRole)input_int("  Cargo", 0, 6);

    /* Nível de acesso padrão por cargo */
    switch (s.role) {
        case ROLE_GERENTE:       s.access_level = ACCESS_GERENTE;    break;
        case ROLE_RECEPCIONISTA: s.access_level = ACCESS_OPERADOR;   break;
        default:                 s.access_level = ACCESS_BASICO;     break;
    }
    printf("  Nivel de acesso sugerido: %d\n", s.access_level);
    if (input_confirm("  Alterar nivel de acesso")) {
        s.access_level = input_int("  Nivel (1=basico .. 5=admin)", 1, 5);
    }

    s.shift = input_int("  Turno [0]Manha [1]Tarde [2]Noite", 0, 2);

    s.salary = input_double("  Salario (R$)", 0.0, 999999.99);

    do {
        input_string("  Data admissao (DD/MM/AAAA): ", s.hire_date, sizeof(s.hire_date));
        if (!date_is_valid(s.hire_date)) printf("  [!] Data invalida.\n");
    } while (!date_is_valid(s.hire_date));

    /* Usuário e senha */
    do {
        input_string("  Nome de usuario (login): ", s.username, sizeof(s.username));
        str_lower(s.username);
        if (staff_find_by_username(s.username) != NULL) {
            printf("  [!] Usuario '%s' ja existe.\n", s.username);
            s.username[0] = '\0';
        }
    } while (s.username[0] == '\0');

    char password[50];
    input_string("  Senha inicial       : ", password, sizeof(password));
    unsigned long h = hash_djb2(password);
    hash_to_hex(h, s.password_hash);

    s.id     = storage_next_staff_id();
    s.active = 1;

    g_staff[g_staff_count++] = s;
    storage_save_staff();

    char log[80];
    snprintf(log, sizeof(log), "STAFF ADD id=%d user=%s role=%s",
             s.id, s.username, staff_role_str(s.role));
    storage_audit_log(g_session.staff.id, log);

    printf("\n  [OK] Funcionario '%s' cadastrado (ID: %d, usuario: %s).\n",
           s.full_name, s.id, s.username);
    pause_screen();
}

void staff_edit(void) {
    if (!auth_require(ACCESS_GERENTE)) return;

    clear_screen();
    print_header("EDITAR FUNCIONARIO");

    int id = input_int("  ID do funcionario", 1, 999999);
    Staff *s = staff_find_by_id(id);
    if (!s) {
        printf("  [!] Funcionario ID %d nao encontrado.\n", id);
        pause_screen();
        return;
    }

    staff_print_full(s);
    printf("  [1]Nome  [2]Depto  [3]Cargo  [4]Nivel  [5]Turno  [6]Salario  [0]Cancelar\n");
    int opt = input_int("  Opcao", 0, 6);

    switch (opt) {
        case 1: input_string("  Novo nome       : ", s->full_name,  sizeof(s->full_name));  break;
        case 2: input_string("  Novo departamento: ", s->department, sizeof(s->department)); break;
        case 3:
            printf("  [0]Recepcionista [1]Gerente [2]Camareiro [3]Manutencao\n");
            printf("  [4]Seguranca     [5]Cozinheiro [6]Garcom\n");
            s->role = (StaffRole)input_int("  Novo cargo", 0, 6);
            break;
        case 4:
            s->access_level = input_int("  Novo nivel (1-5)", 1, 5);
            break;
        case 5:
            s->shift = input_int("  Turno [0]Manha [1]Tarde [2]Noite", 0, 2);
            break;
        case 6:
            s->salary = input_double("  Novo salario (R$)", 0.0, 999999.99);
            break;
        case 0:
            return;
    }

    storage_save_staff();
    char log[80];
    snprintf(log, sizeof(log), "STAFF EDIT id=%d campo=%d", s->id, opt);
    storage_audit_log(g_session.staff.id, log);
    printf("\n  [OK] Funcionario atualizado.\n");
    pause_screen();
}

void staff_change_password(void) {
    /* Qualquer usuário pode trocar sua própria senha;
       gerente pode trocar a de qualquer um */
    clear_screen();
    print_header("TROCAR SENHA");

    int id;
    if (auth_check_access(ACCESS_GERENTE)) {
        printf("  Deixe em branco para alterar sua propria senha.\n");
        char buf[16];
        input_string("  ID do funcionario (ou 0 para si mesmo): ", buf, sizeof(buf));
        id = (buf[0] == '0' || buf[0] == '\0') ? g_session.staff.id : atoi(buf);
    } else {
        id = g_session.staff.id;
    }

    Staff *s = staff_find_by_id(id);
    if (!s) {
        printf("  [!] Funcionario nao encontrado.\n");
        pause_screen();
        return;
    }

    /* Confirma senha atual (não exigido se gerente alterando outro) */
    if (id == g_session.staff.id) {
        char cur[50], cur_hex[20];
        input_string("  Senha atual: ", cur, sizeof(cur));
        hash_to_hex(hash_djb2(cur), cur_hex);
        if (strcmp(cur_hex, s->password_hash) != 0) {
            printf("  [!] Senha atual incorreta.\n");
            pause_screen();
            return;
        }
    }

    char np[50], np2[50];
    input_string("  Nova senha    : ", np,  sizeof(np));
    input_string("  Confirmar senha: ", np2, sizeof(np2));
    if (strcmp(np, np2) != 0) {
        printf("  [!] As senhas nao coincidem.\n");
        pause_screen();
        return;
    }

    hash_to_hex(hash_djb2(np), s->password_hash);

    /* Se trocou a própria senha, atualiza na sessão */
    if (id == g_session.staff.id)
        memcpy(g_session.staff.password_hash, s->password_hash,
               sizeof(s->password_hash));

    storage_save_staff();
    char log[60];
    snprintf(log, sizeof(log), "STAFF PASSWD CHANGE id=%d", s->id);
    storage_audit_log(g_session.staff.id, log);
    printf("\n  [OK] Senha alterada com sucesso.\n");
    pause_screen();
}

void staff_toggle_active(void) {
    if (!auth_require(ACCESS_GERENTE)) return;

    clear_screen();
    print_header("ATIVAR / INATIVAR FUNCIONARIO");

    int id = input_int("  ID do funcionario", 1, 999999);
    if (id == g_session.staff.id) {
        printf("  [!] Voce nao pode inativar sua propria conta.\n");
        pause_screen();
        return;
    }

    Staff *s = staff_find_by_id(id);
    if (!s) {
        printf("  [!] Funcionario nao encontrado.\n");
        pause_screen();
        return;
    }

    printf("  Funcionario: %s  |  Status: %s\n",
           s->full_name, s->active ? "ATIVO" : "INATIVO");
    s->active = s->active ? 0 : 1;

    storage_save_staff();
    char log[60];
    snprintf(log, sizeof(log), "STAFF TOGGLE id=%d active=%d", s->id, s->active);
    storage_audit_log(g_session.staff.id, log);
    printf("\n  [OK] Funcionario agora esta: %s\n",
           s->active ? "ATIVO" : "INATIVO");
    pause_screen();
}

/* =========================================================
 *  Menu do módulo
 * ========================================================= */
static const MenuItem staff_items[] = {
    {1, "Listar funcionarios",    ACCESS_GERENTE, staff_list},
    {2, "Cadastrar funcionario",  ACCESS_GERENTE, staff_add},
    {3, "Editar funcionario",     ACCESS_GERENTE, staff_edit},
    {4, "Ativar / Inativar",      ACCESS_GERENTE, staff_toggle_active},
    {5, "Trocar senha",           ACCESS_BASICO,  staff_change_password},
    {0, "Voltar",                 0,              NULL}
};

void staff_menu(void) {
    menu_run("MODULO: FUNCIONARIOS",
             staff_items,
             (int)(sizeof(staff_items) / sizeof(staff_items[0])));
}
