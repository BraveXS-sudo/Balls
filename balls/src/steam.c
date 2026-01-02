#include "steam.h"
#include "overrides.h"
#include "storage.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Helper to find VDF values
char *find_vdf_value(const char *buffer, const char *key) {
  char search[256];
  snprintf(search, sizeof(search), "\"%s\"", key);
  char *pos = strstr(buffer, search);
  if (!pos)
    return NULL;

  pos += strlen(search);
  while (*pos && (*pos == ' ' || *pos == '\t' || *pos == '\n' || *pos == '\r'))
    pos++;

  if (*pos == '\"') {
    pos++;
    char *end = strchr(pos, '\"');
    if (end) {
      int len = end - pos;
      char *val = malloc(len + 1);
      strncpy(val, pos, len);
      val[len] = 0;
      return val;
    }
  }
  return NULL;
}

AppList load_steam_apps() {
  AppList list = {0};
  list.capacity = 50;
  list.apps = malloc(sizeof(App) * list.capacity);

  char path[1024];
  const char *home = getenv("HOME");
  snprintf(path, sizeof(path), "%s/.local/share/Steam/steamapps", home);

  DIR *d = opendir(path);
  if (d) {
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
      if (strstr(dir->d_name, ".acf")) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, dir->d_name);

        FILE *f = fopen(full_path, "r");
        if (f) {
          fseek(f, 0, SEEK_END);
          long len = ftell(f);
          fseek(f, 0, SEEK_SET);

          char *content = malloc(len + 1);
          fread(content, 1, len, f);
          content[len] = 0;
          fclose(f);

          char *name = find_vdf_value(content, "name");
          char *appid_str = find_vdf_value(content, "appid");

          if (name && appid_str) {
            if (list.count >= list.capacity) {
              list.capacity *= 2;
              list.apps = realloc(list.apps, sizeof(App) * list.capacity);
            }

            App app = {0};
            strncpy(app.name, name, 255);
            app.steam_id = atoi(appid_str);
            app.type = APP_TYPE_STEAM;

            // Default Icon
            snprintf(
                app.image_path, 1023,
                "%s/.local/share/Steam/appcache/librarycache/%d_header.jpg",
                home, app.steam_id);
            if (access(app.image_path, F_OK) != 0) {
              // Fallback to library_600x900.jpg if header missing
              snprintf(app.image_path, 1023,
                       "%s/.local/share/Steam/appcache/librarycache/"
                       "%d_library_600x900.jpg",
                       home, app.steam_id);
            }

            // Apply Overrides
            apply_override(&app);

            list.apps[list.count++] = app;
          }

          if (name)
            free(name);
          if (appid_str)
            free(appid_str);
          free(content);
        }
      }
    }
    closedir(d);
  }

  // Custom Apps (Legacy support/Integration)
  // Note: We are putting custom apps in GAMES category? Or separate?
  // Usually user wanted them separate. But function returns "steam apps".
  // Let's stick to Steam apps here.

  return list;
}
