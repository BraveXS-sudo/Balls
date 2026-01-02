#ifndef UI_H
#define UI_H

#include "app.h"

void InitUI(
    void); // No args, we'll load categories internally or pass a main struct
void UnloadUI(void);
void UpdateUI(void);
void DrawUI(void);

#endif
