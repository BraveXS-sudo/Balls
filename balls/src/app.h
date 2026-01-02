#ifndef APP_H
#define APP_H

#include <stdbool.h>

typedef enum { APP_TYPE_STEAM, APP_TYPE_SYSTEM, APP_TYPE_CUSTOM } AppType;

typedef struct {
  char name[256];
  char exec[1024];
  char image_path[1024];
  int steam_id; // -1 for non-steam
  AppType type;
  bool hidden; // New field
} App;

typedef struct {
  App *apps;
  int count;
  int capacity;
} AppList;

#endif
