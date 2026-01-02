#include "overrides.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char id[512];
  char name[256];
  char icon[1024];
} Override;

static Override *overrides = NULL;
static int override_count = 0;
static int override_cap = 0;

void load_overrides(void) {
  if (overrides)
    free(overrides);
  overrides = NULL;
  override_count = 0;
  override_cap = 0;

  FILE *f = fopen(get_data_path("app_overrides.dat"), "r");
  if (!f)
    return;

  char line[2048];
  while (fgets(line, sizeof(line), f)) {
    if (override_count >= override_cap) {
      override_cap = (override_cap == 0) ? 16 : override_cap * 2;
      overrides = realloc(overrides, sizeof(Override) * override_cap);
    }

    char *id = strtok(line, "|");
    char *name = strtok(NULL, "|");
    char *icon = strtok(NULL, "\n");

    if (id && name && icon) {
      strncpy(overrides[override_count].id, id, 511);
      strncpy(overrides[override_count].name, name, 255);
      strncpy(overrides[override_count].icon, icon, 1023);
      override_count++;
    }
  }
  fclose(f);
}

void save_override(const char *id, const char *name, const char *icon_path) {
  // Update or Add
  int found = -1;
  for (int i = 0; i < override_count; i++) {
    if (strcmp(overrides[i].id, id) == 0) {
      found = i;
      break;
    }
  }

  if (found != -1) {
    strncpy(overrides[found].name, name, 255);
    strncpy(overrides[found].icon, icon_path, 1023);
  } else {
    if (override_count >= override_cap) {
      override_cap = (override_cap == 0) ? 16 : override_cap * 2;
      overrides = realloc(overrides, sizeof(Override) * override_cap);
    }
    strncpy(overrides[override_count].id, id, 511);
    strncpy(overrides[override_count].name, name, 255);
    strncpy(overrides[override_count].icon, icon_path, 1023);
    override_count++;
  }

  // Save to disk
  FILE *f = fopen(get_data_path("app_overrides.dat"), "w");
  if (f) {
    for (int i = 0; i < override_count; i++) {
      fprintf(f, "%s|%s|%s\n", overrides[i].id, overrides[i].name,
              overrides[i].icon);
    }
    fclose(f);
  }
}

// Hidden State Persistence
static char **hidden_ids = NULL;
static int hidden_count = 0;
static int hidden_cap = 0;

void load_hidden_states(void) {
  if (hidden_ids) {
    for (int i = 0; i < hidden_count; i++)
      free(hidden_ids[i]);
    free(hidden_ids);
  }
  hidden_ids = NULL;
  hidden_count = 0;
  hidden_cap = 0;

  FILE *f = fopen(get_data_path("hidden.dat"), "r");
  if (!f)
    return;

  char line[512];
  while (fgets(line, sizeof(line), f)) {
    line[strcspn(line, "\n")] = 0;
    if (strlen(line) > 0) {
      if (hidden_count >= hidden_cap) {
        hidden_cap = (hidden_cap == 0) ? 16 : hidden_cap * 2;
        hidden_ids = realloc(hidden_ids, sizeof(char *) * hidden_cap);
      }
      hidden_ids[hidden_count++] = strdup(line);
    }
  }
  fclose(f);
}

void save_hidden_state(const char *id, bool hidden) {
  // Update Memory
  int found = -1;
  for (int i = 0; i < hidden_count; i++) {
    if (strcmp(hidden_ids[i], id) == 0) {
      found = i;
      break;
    }
  }

  if (hidden && found == -1) {
    // Add
    if (hidden_count >= hidden_cap) {
      hidden_cap = (hidden_cap == 0) ? 16 : hidden_cap * 2;
      hidden_ids = realloc(hidden_ids, sizeof(char *) * hidden_cap);
    }
    hidden_ids[hidden_count++] = strdup(id);
  } else if (!hidden && found != -1) {
    // Remove
    free(hidden_ids[found]);
    for (int i = found; i < hidden_count - 1; i++) {
      hidden_ids[i] = hidden_ids[i + 1];
    }
    hidden_count--;
  }

  // Save to Disk
  FILE *f = fopen(get_data_path("hidden.dat"), "w");
  if (f) {
    for (int i = 0; i < hidden_count; i++) {
      fprintf(f, "%s\n", hidden_ids[i]);
    }
    fclose(f);
  }
}

bool is_app_hidden(const char *id) {
  for (int i = 0; i < hidden_count; i++) {
    if (strcmp(hidden_ids[i], id) == 0)
      return true;
  }
  return false;
}

void get_app_id(App *app, char *buffer, size_t size) {
  if (app->steam_id != -1) {
    snprintf(buffer, size, "steam_%d", app->steam_id);
  } else {
    // Use exec path as ID for system apps
    strncpy(buffer, app->exec, size - 1);
    buffer[size - 1] = 0;
  }
}

void apply_override(App *app) {
  if (!app)
    return;
  char id[1024];
  get_app_id(app, id, sizeof(id));

  // Apply Hidden
  app->hidden = is_app_hidden(id);

  // Apply Name/Icon
  for (int i = 0; i < override_count; i++) {
    if (strcmp(overrides[i].id, id) == 0) {
      strncpy(app->name, overrides[i].name, 255);
      strncpy(app->image_path, overrides[i].icon, 1023);
      return;
    }
  }
}
