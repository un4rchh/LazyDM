#ifndef TERM_H
#define TERM_H

void term_fill_bg(int fd, int color);
void term_bg_color(int fd, int color);
void term_fg_color(int fd, int color);
void term_clear(int fd);
void term_goto(int fd, int y, int x);
void term_color(int fd, int color);
void term_reset(int fd);
void term_box(int fd, int x, int y, int w, int h);
void term_center(int fd, int w, int y, char *str);
void draw_err(int fd, int by, int bh, int bx, int bw, const char *msg);

#endif 
