#include "app.h"
#include <stdlib.h>

void free_app_list(AppList *list) {
  if (list->apps) {
    free(list->apps);
    list->apps = NULL;
  }
  list->count = 0;
}
