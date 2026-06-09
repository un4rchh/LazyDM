#ifndef AUTH_H
#define AUTH_H

#include <security/pam_appl.h>

const char *auth_user(const char *login, char *passwd, pam_handle_t **pamh, const char *service);

#endif
