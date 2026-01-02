#ifndef DESKTOP_ENTRY_H
#define DESKTOP_ENTRY_H

#include "app.h"

// Scans /usr/share/applications for .desktop files
AppList scan_system_apps(void);

#endif
