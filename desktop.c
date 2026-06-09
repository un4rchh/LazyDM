#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#include "desktop.h"

static void cut_off(char *line, Session *s);

void load_session(Session *sessions, int *count, const char *dirpath, int is_wayland){
  (void)is_wayland;
  DIR *dir = opendir(dirpath);
  if (!dir){ return; }
  
  struct dirent *entry;
  
  Session s = { 0 };

  while ((entry = readdir(dir)) != NULL){
    size_t len = strlen(entry->d_name);

    if (len >= 8 && strstr(entry->d_name, ".desktop") != NULL){
      char path[PATH_MAX];
      snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
      FILE *fp = fopen(path, "r");

      if (!fp){
        continue;
      }
    
      memset(&s, 0, sizeof(Session));

      char line[PATH_MAX];
      while(fgets(line, sizeof(line), fp) != NULL){
        if (strncmp(line, "Name=", 5) == 0 || strncmp(line, "Exec=", 5) == 0){
          cut_off(line, &s); 
        }
      }
      sessions[*count] = s;
      (*count)++;
      if (*count >= MAX_SES) { break; }
      fclose(fp);
    }
  }
  closedir(dir);
}

void cut_off(char *line, Session *s){
  char *eq = strchr(line, '=');
  if (!eq) { return; }
  char *value = eq+1;

  value[strcspn(value, "\r\n")] = '\0';

  size_t len = strlen(value);
  while (len > 0 && (value[len-1] == ' ' || value[len-1] == '\t' || value[len-1] == '\r' || value[len-1] == '\n')){
    len--;
    value[len] = '\0';
  }

  if (strncmp(line, "Name=", 5) == 0){
      strncpy(s->name, value, BUF_SIZE-1);
      s->name[BUF_SIZE-1] = '\0';
    } else if (strncmp(line, "Exec=", 5) == 0) {
        strncpy(s->exec, value, PATH_MAX-1);
        s->exec[PATH_MAX-1] = '\0';
    }
}
