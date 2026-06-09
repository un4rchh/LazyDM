#ifndef DESKTOP_H
#define DESKTOP_H

#include <limits.h>
#include "config.h"

typedef struct {
  char name[BUF_SIZE];
  char exec[PATH_MAX];
} Session;

void load_session(Session *sessions, int *count, const char *dirpath, int is_wayland);

#endif
