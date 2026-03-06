#ifndef AUTH_H
#define AUTH_H

#include "types.h"

typedef struct {
    int   logged_in;
    Staff staff;       /* cópia do funcionário logado na sessão */
} Session;

extern Session g_session;

/* Exibe tela de login; retorna 1 se autenticado, 0 após 3 falhas */
int  auth_login(void);
void auth_logout(void);

/* Retorna 1 se a sessão tem nível >= required_level */
int  auth_check_access(int required_level);

/* Como auth_check_access, mas exibe mensagem e pause se negado */
int  auth_require(int required_level);

#endif /* AUTH_H */
