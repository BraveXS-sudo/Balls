#include "desktop_entry.h"
#include "overrides.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AppList scan_system_apps() {
  AppList list = {0};
  list.capacity = 50;
  list.apps = malloc(sizeof(App) * list.capacity);

  const char *dirs[] = {"/usr/share/applications",
                        "/usr/local/share/applications", NULL};

  for (int i = 0; dirs[i]; i++) {
    DIR *d = opendir(dirs[i]);
    if (!d)
      continue;

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
      if (strstr(dir->d_name, ".desktop")) {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirs[i], dir->d_name);

        FILE *f = fopen(full_path, "r");
        if (f) {
          char line[512];
          char name[256] = {0};
          char exec[512] = {0};
          char icon[512] = {0};
          int is_app = 0;

          while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "Name=", 5) == 0 && name[0] == 0) {
              strncpy(name, line + 5, 255);
              name[strcspn(name, "\n")] = 0;
            }
            if (strncmp(line, "Exec=", 5) == 0 && exec[0] == 0) {
              strncpy(exec, line + 5, 511);
              exec[strcspn(exec, "\n")] = 0;
              // Strip args like %u
              char *space = strstr(exec, " %");
              if (space)
                *space = 0;
            }
            if (strncmp(line, "Icon=", 5) == 0 && icon[0] == 0) {
              strncpy(icon, line + 5, 511);
              icon[strcspn(icon, "\n")] = 0;
            }
            if (strncmp(line, "Type=Application", 16) == 0)
              is_app = 1;
            if (strncmp(line, "NoDisplay=true", 14) == 0)
              is_app = 0;
          }
          fclose(f);

          if (is_app && name[0] && exec[0]) {
            if (list.count >= list.capacity) {
              list.capacity *= 2;
              list.apps = realloc(list.apps, sizeof(App) * list.capacity);
            }

            App app = {0};
            strncpy(app.name, name, 255);
            strncpy(app.exec, exec, 1023);

            // Icon Resolution (simplified)
            // If path, use it. If name, try to find in pixmaps or hicolor.
            if (icon[0] == '/') {
              strncpy(app.image_path, icon, 1023);
            } else {
              // Try common paths
              snprintf(app.image_path, 1023, "/usr/share/pixmaps/%s.png", icon);
              FILE *test = fopen(app.image_path, "r");
              if (test)
                fclose(test);
              else {
                // Try hicolor (very basic fallback)
                snprintf(app.image_path, 1023,
                         "/usr/share/icons/hicolor/48x48/apps/%s.png", icon);
              }
            }

            app.type = APP_TYPE_SYSTEM;
            app.steam_id = -1;

            // Apply Overrides
            apply_override(&app);

            list.apps[list.count++] = app;
          }
        }
      }
    }
    closedir(d);
  }
  return list;
}
