#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

// Returns a static buffer containing ~/.local/share/balls/filename
// Ensures directory exists.
const char *get_data_path(const char *filename);

#endif
