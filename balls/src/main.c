#include "overrides.h"
#include "raylib.h"
#include "ui.h"

int main(void) {
  const int screenWidth = 1280;
  const int screenHeight = 720;

  InitWindow(screenWidth, screenHeight, "BALLS");
  SetTargetFPS(60);

  // Init Overrides first
  load_overrides();

  InitUI();

  // Main loop
  while (!WindowShouldClose()) {
    UpdateUI();

    BeginDrawing();
    ClearBackground(GetColor(0x181818ff));
    DrawUI();
    EndDrawing();
  }

  UnloadUI();
  CloseWindow();

  return 0;
}
