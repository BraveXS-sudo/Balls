#ifndef CATEGORIES_H
#define CATEGORIES_H

#include "app.h"
#include <stdbool.h>

typedef struct {
  char name[64];
  AppList apps;
  bool is_default;
  bool has_plus_button;
} Category;

typedef struct {
  Category *categories;
  int count;
  int capacity;
} CategoryList;

void init_categories(void);
CategoryList *get_categories(void);
// Deprecated management functions kept as no-ops
void add_category(const char *name);
void rename_category(int index, const char *new_name);
void delete_category(int index);
void save_categories(void);

#endif
