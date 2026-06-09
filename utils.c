#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

#include "utils.h"
#include "term.h"
#include "config.h"

static size_t get_lenght(const char *str);

void strip_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[len-1] = '\0';
        len--;
    }
}

char *get_abs_path(const char *cmd) {
    if (strchr(cmd, '/') != NULL) return strdup(cmd);
    char full_path[PATH_MAX];
    for (int i = 0; bin_path[i] != NULL; i++) {
        snprintf(full_path, sizeof(full_path), "%s/%s", bin_path[i], cmd);
        if (access(full_path, X_OK) == 0) return strdup(full_path);
    }
    return strdup(cmd);
}

size_t get_lenght(const char *str){
  size_t len = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\033' && str[i+1] == '[') {
        while (str[i] != '\0' && str[i] != 'm') { i++; }
    } else { len++; }
  }
  return len;
}

void draw_motd(int fd, const char *path, int box_y, int box_x, int box_w){
  FILE *fp = popen(path, "r");
  if (!fp) return;

  char lines[64][BUF_SIZE];
  int line_count = 0;

  while(line_count < 64 && fgets(lines[line_count], sizeof(lines[0]), fp) != 0){
    lines[line_count][strcspn(lines[line_count], "\n\r")] = '\0';
    if (get_lenght(lines[line_count]) > 0 || line_count == 0){
      line_count++;
    }
  }
  pclose(fp);
  if (line_count == 0) return;

  size_t max_len = 0;
  for (int i = 0; i < line_count; i++){
    size_t v_len = get_lenght(lines[i]);
    if (v_len > max_len){ max_len = v_len; }
  }

  int block_offset = (box_w - max_len)/2;
  if (block_offset < 0){ block_offset = 0; }

  int motd_x = box_x + block_offset + 3;
  
  int tar_bot = box_y - 5;
  int start_y = tar_bot - (line_count - 1);
  if (start_y < 1){ start_y = 1; }

  for (int i = 0; i < line_count; i++){
    size_t len = get_lenght(lines[i]);
    if (len == 0) continue;

    term_goto(fd, start_y + i, motd_x);
    write(fd, lines[i], strlen(lines[i]));
  }
}
