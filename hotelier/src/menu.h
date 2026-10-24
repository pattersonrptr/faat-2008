#ifndef MENU_H
#define MENU_H

#define MENU_MAX_ITEMS 20

typedef struct {
    int   key;            /* número que o usuário pressiona          */
    char  label[80];
    int   min_access;     /* nível mínimo de acesso (0 = sem restrição) */
    void  (*action)(void);/* NULL + key==0 → sair/voltar do menu     */
} MenuItem;

/*
 * Exibe o menu em loop e executa a action correspondente à tecla escolhida.
 * O loop encerra quando o usuário escolhe um item com action == NULL e key == 0.
 * Itens com action == NULL e key != 0 exibem "módulo em desenvolvimento".
 */
void menu_run(const char *title, const MenuItem *items, int count);

#endif /* MENU_H */
