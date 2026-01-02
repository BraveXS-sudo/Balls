#include "storage.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For custom apps that were manually added.
// Note: In v6 we disabled adding custom apps via UI, but loading existing ones
// or future ones is fine.

void save_custom_app(App app) {
  FILE *f = fopen(get_data_path("custom_apps.dat"), "a");
  if (f) {
    fprintf(f, "%s|%s|%s\n", app.name, app.exec, app.image_path);
    fclose(f);
  }
}

AppList load_custom_apps() {
  AppList list = {0};
  list.capacity = 10;
  list.apps = malloc(sizeof(App) * list.capacity);

  FILE *f = fopen(get_data_path("custom_apps.dat"), "r");
  if (f) {
    char line[2048];
    while (fgets(line, sizeof(line), f)) {
      char *name = strtok(line, "|");
      char *exec = strtok(NULL, "|");
      char *img = strtok(NULL, "\n");

      if (name && exec && img) {
        if (list.count >= list.capacity) {
          list.capacity *= 2;
          list.apps = realloc(list.apps, sizeof(App) * list.capacity);
        }

        App app = {0};
        strncpy(app.name, name, 255);
        strncpy(app.exec, exec, 1023);
        strncpy(app.image_path, img, 1023);
        app.type = APP_TYPE_CUSTOM;
        app.steam_id = -1;

        list.apps[list.count++] = app;
      }
    }
    fclose(f);
  }

  return list;
}
