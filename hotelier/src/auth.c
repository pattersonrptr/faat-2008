#include "auth.h"
#include "storage.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

Session g_session = {0};

int auth_login(void) {
    char username[30], password[50], hex[20];
    int  attempts = 0;

    while (attempts < 3) {
        clear_screen();
        print_header("LOGIN DO SISTEMA");
        input_string("  Usuario: ", username, sizeof(username));
        input_string("  Senha  : ", password, sizeof(password));

        unsigned long h = hash_djb2(password);
        hash_to_hex(h, hex);

        for (int i = 0; i < g_staff_count; i++) {
            if (g_staff[i].active
                && strcmp(g_staff[i].username,      username) == 0
                && strcmp(g_staff[i].password_hash, hex)      == 0)
            {
                g_session.logged_in = 1;
                g_session.staff     = g_staff[i];
                storage_audit_log(g_session.staff.id, "LOGIN");
                return 1;
            }
        }

        attempts++;
        printf("\n  [!] Credenciais invalidas. Tentativa %d/3.\n", attempts);
        pause_screen();
    }

    printf("\n  [!] Numero maximo de tentativas atingido.\n");
    pause_screen();
    return 0;
}

void auth_logout(void) {
    if (g_session.logged_in) {
        storage_audit_log(g_session.staff.id, "LOGOUT");
        memset(&g_session, 0, sizeof(g_session));
    }
}

int auth_check_access(int required_level) {
    return g_session.logged_in
        && g_session.staff.access_level >= required_level;
}

int auth_require(int required_level) {
    if (!auth_check_access(required_level)) {
        printf("\n  [!] Acesso negado. Nivel minimo: %d  |  Seu nivel: %d\n",
               required_level,
               g_session.logged_in ? g_session.staff.access_level : 0);
        pause_screen();
        return 0;
    }
    return 1;
}
