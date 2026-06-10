#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

#define WRITE(fd, buf, len) \
  do { \
    if (write((fd), (buf), (len)) < 0) { \
        perror("write failed"); \
    } \
  } while(0)

void strip_newline(char *str);
char *get_abs_path(const char *cmd);
void draw_motd(int fd, const char *path, int box_y, int box_x, int box_w);

#endif
