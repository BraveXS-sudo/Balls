#ifndef OVERRIDES_H
#define OVERRIDES_H

#include "app.h"
#include <stddef.h>

void load_overrides(void);
void load_hidden_states(void);
void save_override(const char *id, const char *name, const char *icon_path);
void save_hidden_state(const char *id, bool hidden);
bool is_app_hidden(const char *id);
void apply_override(App *app);
void get_app_id(App *app, char *buffer, size_t size);

#endif
