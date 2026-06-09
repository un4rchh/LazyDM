#ifndef SESSION_H
#define SESSION_H

#include "stdbool.h"
#include "config.h"
#include "desktop.h"

typedef struct {
  int fd;
  pam_handle_t *pamh;
  struct passwd *pw;
  const char *cmd;
} Start;

int start_session(Start *s);
bool prepare_session(Config *lz, Start *s, Session *sessions, int num, int by, int bx, int bh, int bw, const int *xcount, const int *wcount);

#endif
