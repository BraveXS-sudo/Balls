#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char path_buffer[1024];

const char *get_data_path(const char *filename) {
  const char *home = getenv("HOME");
  if (!home)
    return filename; // Fallback

  // Construct base dir ~/.local/share/balls
  snprintf(path_buffer, sizeof(path_buffer), "%s/.local/share/balls", home);

  // Create dir if needed
  struct stat st = {0};
  if (stat(path_buffer, &st) == -1) {
    // Try making parent .local/share first if needed (simplified)
    char parent[1024];
    snprintf(parent, sizeof(parent), "%s/.local/share", home);
    mkdir(parent, 0700);
    mkdir(path_buffer, 0700);
  }

  // Append filename
  snprintf(path_buffer, sizeof(path_buffer), "%s/.local/share/balls/%s", home,
           filename);
  return path_buffer;
}
