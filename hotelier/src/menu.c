#include "menu.h"
#include "auth.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

void menu_run(const char *title, const MenuItem *items, int count) {
    char buf[16];
    int  choice;

    do {
        clear_screen();
        print_header(title);

        if (g_session.logged_in) {
            printf("  Operador: %-30s | Nivel: %d\n\n",
                   g_session.staff.full_name,
                   g_session.staff.access_level);
        }

        for (int i = 0; i < count; i++) {
            if (items[i].min_access == 0
                || auth_check_access(items[i].min_access))
            {
                printf("  [%d] %s\n", items[i].key, items[i].label);
            }
        }

        printf("\n  Opcao: ");
        fflush(stdout);

        if (fgets(buf, sizeof(buf), stdin) == NULL) continue;
        if (sscanf(buf, "%d", &choice) != 1)        continue;

        int found = 0;
        for (int i = 0; i < count; i++) {
            if (items[i].key != choice) continue;
            found = 1;

            /* key == 0 com action NULL → voltar/sair */
            if (items[i].action == NULL && items[i].key == 0) return;

            /* outro item sem action → módulo em desenvolvimento */
            if (items[i].action == NULL) {
                printf("\n  [!] Modulo em desenvolvimento.\n");
                pause_screen();
                break;
            }

            /* verifica acesso */
            if (items[i].min_access > 0
                && !auth_check_access(items[i].min_access))
            {
                printf("\n  [!] Acesso negado. Nivel requerido: %d\n",
                       items[i].min_access);
                pause_screen();
            } else {
                items[i].action();
            }
            break;
        }

        if (!found) {
            printf("  Opcao invalida.\n");
            pause_screen();
        }
    } while (1);
}
