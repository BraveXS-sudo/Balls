#include "categories.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CategoryList cat_list = {0};

void init_categories(void) {
  // Allocation
  if (cat_list.categories)
    free(cat_list.categories);
  cat_list.capacity = 3;
  cat_list.count = 3;
  cat_list.categories = malloc(sizeof(Category) * cat_list.capacity);

  // 0: GAMES (Steam)
  strcpy(cat_list.categories[0].name, "GAMES");
  cat_list.categories[0].is_default = true;
  cat_list.categories[0].has_plus_button = false;

  // 1: APPS (System)
  strcpy(cat_list.categories[1].name, "APPS");
  cat_list.categories[1].is_default = true;
  cat_list.categories[1].has_plus_button = false;

  // 2: HIDDEN
  strcpy(cat_list.categories[2].name, "HIDDEN");
  cat_list.categories[2].is_default = true;
  cat_list.categories[2].has_plus_button = false;
  cat_list.categories[2].apps.count = 0;
  cat_list.categories[2].apps.capacity = 10;
  cat_list.categories[2].apps.apps = malloc(sizeof(App) * 10);
}

CategoryList *get_categories(void) { return &cat_list; }

// Stubs
void add_category(const char *name) {}
void rename_category(int index, const char *new_name) {}
void delete_category(int index) {}
void save_categories(void) {}
