#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

void strip_newline(char *str);
char *get_abs_path(const char *cmd);
void draw_motd(int fd, const char *path, int box_y, int box_x, int box_w);

#endif
