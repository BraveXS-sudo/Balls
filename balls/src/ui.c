#include "ui.h"
#include "categories.h"
#include "desktop_entry.h"
#include "overrides.h"
#include "raylib.h"
#include "steam.h"
#include "storage.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PADDING 20
#define CARD_HEIGHT 280
#define CARD_WIDTH 180
#define TOP_BAR_HEIGHT 60
#define SIDEBAR_WIDTH_PERCENT 0.2f

// Audio State
static Sound snd_hover;
static Sound snd_launch;
static Sound snd_switch;
static bool audio_init = false;

// State
static int current_category_idx = 0;

static Texture2D *current_textures = NULL;
static int current_texture_count = 0;

static char search_query[256] = {0};
static bool search_active = false;
static int scroll_offset = 0;

// Modal
static bool show_modal = false;
static bool is_edit_mode = false;
static App *app_to_edit = NULL; // Pointer to app being edited
static char input_name[256] = {0};
static char input_path[1024] = {0};
static char input_img[1024] = {0};
static int active_input = 0;

// Manual Wave Generation Helper
Wave GenerateSineWave(float freq, int sampleRate, float duration) {
  Wave wave = {0};
  wave.sampleRate = sampleRate;
  wave.sampleSize = 16;
  wave.channels = 1;
  wave.frameCount = (int)(sampleRate * duration);

  short *data = (short *)malloc(wave.frameCount * sizeof(short));

  for (int i = 0; i < wave.frameCount; i++) {
    data[i] = (short)(32000.0f * sinf(2.0f * PI * freq * i / sampleRate));
  }

  wave.data = data;
  return wave;
}

void GenerateSounds() {
  InitAudioDevice();
  if (!IsAudioDeviceReady()) {
    printf("ERROR: Audio device could not be initialized.\n");
    return;
  }
  SetMasterVolume(1.0f); // Force max volume
  audio_init = true;

  // Hover: High tiny blip
  Wave w = GenerateSineWave(800.0f, 44100, 0.05f);
  snd_hover = LoadSoundFromWave(w);
  UnloadWave(w);

  // Launch: Power up
  w = GenerateSineWave(220.0f, 44100, 0.2f);
  snd_launch = LoadSoundFromWave(w);
  UnloadWave(w);

  // Switch: Clicky Thud
  w = GenerateSineWave(300.0f, 44100, 0.1f);
  snd_switch = LoadSoundFromWave(w);
  UnloadWave(w);
}

// Master Reload
// 1. Loads all Steam Apps
// 2. Scans all System Apps
// 3. Sorts them into GAMES, APPS, or HIDDEN based on flags
void ReloadAllApps() {
  CategoryList *cats = get_categories();

  // Clear Existing
  for (int c = 0; c < 3; c++) {
    if (cats->categories[c].apps.apps)
      free(cats->categories[c].apps.apps);
    cats->categories[c].apps.count = 0;
    cats->categories[c].apps.capacity = 10;
    cats->categories[c].apps.apps = malloc(sizeof(App) * 10);
  }

  // Load Raw
  AppList raw_steam = load_steam_apps();
  AppList raw_sys = scan_system_apps();

  // Distribute
  // Steam Apps
  for (int i = 0; i < raw_steam.count; i++) {
    int target_cat = 0; // Default GAMES
    if (raw_steam.apps[i].hidden)
      target_cat = 2; // HIDDEN

    AppList *t = &cats->categories[target_cat].apps;
    if (t->count >= t->capacity) {
      t->capacity *= 2;
      t->apps = realloc(t->apps, sizeof(App) * t->capacity);
    }
    t->apps[t->count++] = raw_steam.apps[i];
  }
  free(raw_steam.apps);

  // System Apps
  for (int i = 0; i < raw_sys.count; i++) {
    int target_cat = 1; // Default APPS
    if (raw_sys.apps[i].hidden)
      target_cat = 2; // HIDDEN

    AppList *t = &cats->categories[target_cat].apps;
    if (t->count >= t->capacity) {
      t->capacity *= 2;
      t->apps = realloc(t->apps, sizeof(App) * t->capacity);
    }
    t->apps[t->count++] = raw_sys.apps[i];
  }
  free(raw_sys.apps);
}

void ReloadCurrentTextures() {
  if (current_textures) {
    for (int i = 0; i < current_texture_count; i++)
      if (current_textures[i].id != 0)
        UnloadTexture(current_textures[i]);
    free(current_textures);
  }

  CategoryList *cats = get_categories();
  AppList *list = &cats->categories[current_category_idx].apps;

  current_texture_count = list->count;
  current_textures = malloc(sizeof(Texture2D) * current_texture_count);

  for (int i = 0; i < list->count; i++) {
    current_textures[i] = (Texture2D){0};
    if (list->apps[i].image_path[0] != '\0') {
      Image img = LoadImage(list->apps[i].image_path);
      if (img.data != NULL) {
        ImageResize(&img, CARD_WIDTH, CARD_HEIGHT);
        current_textures[i] = LoadTextureFromImage(img);
        if (current_textures[i].id != 0)
          SetTextureFilter(current_textures[i], TEXTURE_FILTER_BILINEAR);
        UnloadImage(img);
      }
    }
  }
}

void InitUI(void) {
  GenerateSounds();
  load_hidden_states(); // NEW
  init_categories();

  ReloadAllApps(); // NEW Logic

  // Load initial textures
  ReloadCurrentTextures();
}

void UnloadUI(void) {
  if (current_textures) {
    for (int i = 0; i < current_texture_count; i++)
      if (current_textures[i].id != 0)
        UnloadTexture(current_textures[i]);
    free(current_textures);
  }

  if (audio_init) {
    UnloadSound(snd_hover);
    UnloadSound(snd_launch);
    UnloadSound(snd_switch);
    CloseAudioDevice();
  }
}

int str_contains_ci(const char *haystack, const char *needle) {
  if (!needle || !haystack)
    return 0;
  if (strlen(needle) == 0)
    return 1;
  char h[256], n[256];
  strncpy(h, haystack, 255);
  h[255] = 0;
  strncpy(n, needle, 255);
  n[255] = 0;
  for (int i = 0; h[i]; i++)
    h[i] = tolower(h[i]);
  for (int i = 0; n[i]; i++)
    n[i] = tolower(n[i]);
  return strstr(h, n) != NULL;
}

void OpenFilePicker(char *buffer) {
  FILE *f = popen("zenity --file-selection --title=\"Select Icon\" "
                  "--file-filter=\"*.png *.jpg\" 2>/dev/null",
                  "r");
  if (f) {
    char path[1024];
    if (fgets(path, sizeof(path), f)) {
      path[strcspn(path, "\n")] = 0;
      strncpy(buffer, path, 1023);
    }
    pclose(f);
  }
}

void DrawTextBox(int x, int y, int w, int h, char *buffer, int max_len,
                 bool active) {
  DrawRectangle(x, y, w, h,
                active ? GetColor(0x444444ff) : GetColor(0x333333ff));
  DrawRectangleLines(x, y, w, h, active ? SKYBLUE : GRAY);
  DrawText(buffer, x + 5, y + 8, 20, WHITE);
  if (active) {
    int key = GetCharPressed();
    while (key > 0) {
      int len = strlen(buffer);
      if (len < max_len - 1 && key >= 32 && key <= 126) {
        buffer[len] = (char)key;
        buffer[len + 1] = 0;
      }
      key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = strlen(buffer);
      if (len > 0)
        buffer[len - 1] = 0;
    }
  }
}

void UpdateUI(void) {
  if (IsKeyPressed(KEY_ESCAPE) && show_modal)
    show_modal = false;

  if (!show_modal) {
    int wheel = GetMouseWheelMove();
    if (wheel != 0) {
      scroll_offset -= wheel * 40;
      if (scroll_offset < 0)
        scroll_offset = 0;
    }
  }
}

void DrawnModal(int screen_width, int screen_height) {
  DrawRectangle(0, 0, screen_width, screen_height, Fade(BLACK, 0.9f));
  int mw = 600;
  int mh = 650;
  int mx = (screen_width - mw) / 2;
  int my = (screen_height - mh) / 2;

  DrawRectangle(mx, my, mw, mh, GetColor(0x333333ff));
  DrawRectangleLines(mx, my, mw, mh, SKYBLUE);

  DrawText(is_edit_mode ? "EDIT APP" : "ADD NEW APP", mx + 20, my + 20, 30,
           WHITE);

  DrawText("Name:", mx + 50, my + 70, 20, GRAY);
  DrawTextBox(mx + 50, my + 95, 500, 30, input_name, 255, active_input == 0);

  DrawText("Exec:", mx + 50, my + 135, 20, GRAY);
  DrawTextBox(mx + 50, my + 160, 500, 30, input_path, 1023, active_input == 1);

  DrawText("Image:", mx + 50, my + 200, 20, GRAY);
  DrawTextBox(mx + 50, my + 225, 400, 30, input_img, 1023, active_input == 2);

  // Browse Button
  DrawRectangle(mx + 460, my + 225, 90, 30, GRAY);
  DrawText("Browse", mx + 470, my + 232, 20, BLACK);
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (CheckCollisionPointRec(GetMousePosition(),
                               (Rectangle){mx + 460, my + 225, 90, 30})) {
      PlaySound(snd_switch);
      OpenFilePicker(input_img);
    }
  }

  // HIDE TOGGLE
  bool is_currently_hidden = app_to_edit ? app_to_edit->hidden : false;
  Color toggle_col = is_currently_hidden ? RED : DARKBLUE;
  const char *toggle_text = is_currently_hidden ? "UNHIDE APP" : "HIDE APP";

  DrawRectangle(mx + 50, my + 300, 200, 40, toggle_col);
  DrawText(toggle_text, mx + 70, my + 310, 20, WHITE);

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      CheckCollisionPointRec(GetMousePosition(),
                             (Rectangle){mx + 50, my + 300, 200, 40})) {
    if (app_to_edit) {
      char id[1024];
      get_app_id(app_to_edit, id, sizeof(id));
      bool new_val = !app_to_edit->hidden;
      save_hidden_state(id, new_val);
      app_to_edit->hidden = new_val;
      ReloadAllApps(); // Re-sort
      ReloadCurrentTextures();
      show_modal = false; // Close on toggle
      PlaySound(snd_launch);
      return;
    }
  }

  // SAVE Button
  DrawRectangle(mx + 300, my + 300, 250, 40, DARKGREEN);
  DrawText("SAVE & CLOSE", mx + 350, my + 310, 20, WHITE);
  bool clicked_save =
      (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
       CheckCollisionPointRec(GetMousePosition(),
                              (Rectangle){mx + 300, my + 300, 250, 40}));

  // Save
  if (IsKeyPressed(KEY_ENTER) || clicked_save) {
    if (is_edit_mode && app_to_edit) {
      strncpy(app_to_edit->name, input_name, 255);
      strncpy(app_to_edit->image_path, input_img, 1023);

      char id[1024];
      get_app_id(app_to_edit, id, sizeof(id));
      save_override(id, input_name, input_img);

      PlaySound(snd_launch);
      ReloadCurrentTextures();
    }
    show_modal = false;
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    Vector2 m = GetMousePosition();
    if (CheckCollisionPointRec(m, (Rectangle){mx + 50, my + 95, 500, 30}))
      active_input = 0;
    else if (CheckCollisionPointRec(m, (Rectangle){mx + 50, my + 160, 500, 30}))
      active_input = 1;
    else if (CheckCollisionPointRec(m, (Rectangle){mx + 50, my + 225, 500, 30}))
      active_input = 2;
  }
}

void DrawUI(void) {
  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();

  CategoryList *cats = get_categories();

  int sidebar_width = screen_width * SIDEBAR_WIDTH_PERCENT;
  int content_width = screen_width - sidebar_width;

  // Sidebar
  DrawRectangle(0, 0, sidebar_width, screen_height, GetColor(0x222222ff));
  DrawText("BALLS", 20, 20, 40, SKYBLUE);

  // Categories List
  int cat_y = 100;
  for (int i = 0; i < cats->count; i++) {
    // Highlight HIDDEN specially
    Color active_col = GetColor(0x333333ff);
    if (i == 2)
      active_col = GetColor(0x220000ff);

    Rectangle cat_rect = {0, cat_y, sidebar_width, 50};
    bool active = (i == current_category_idx);
    bool hovered = CheckCollisionPointRec(GetMousePosition(), cat_rect);

    if (active)
      DrawRectangleRec(cat_rect, active_col);
    else if (hovered)
      DrawRectangleRec(cat_rect, (i == 2) ? RED : GetColor(0x2a2a2aff));

    DrawText(cats->categories[i].name, 30, cat_y + 15, 20,
             active ? WHITE : GRAY);

    if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (current_category_idx != i) {
        current_category_idx = i;
        PlaySound(snd_switch);
        scroll_offset = 0;
        ReloadCurrentTextures();
      }
    }
    cat_y += 60;
  }

  // Content Area
  int start_x = sidebar_width + PADDING;
  int start_y = TOP_BAR_HEIGHT + PADDING;

  // Top Bar (Search)
  DrawRectangle(sidebar_width, 0, content_width, TOP_BAR_HEIGHT,
                GetColor(0x111111ff));
  DrawText("Search:", sidebar_width + 20, 20, 20, GRAY);
  DrawTextBox(sidebar_width + 100, 15, 400, 30, search_query, 255,
              search_active);

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (CheckCollisionPointRec(GetMousePosition(),
                               (Rectangle){sidebar_width + 100, 15, 400, 30}))
      search_active = true;
    else
      search_active = false;
  }

  // Grid
  AppList *active_list = &cats->categories[current_category_idx].apps;

  int available_w = content_width - (PADDING * 2);
  int cols = available_w / (CARD_WIDTH + PADDING);
  if (cols < 1)
    cols = 1;

  BeginScissorMode(sidebar_width, TOP_BAR_HEIGHT, content_width,
                   screen_height - TOP_BAR_HEIGHT);

  int visible_idx = 0;

  // Draw Apps
  for (int i = 0; i < active_list->count; i++) {
    if (strlen(search_query) > 0 &&
        !str_contains_ci(active_list->apps[i].name, search_query))
      continue;

    int col = visible_idx % cols;
    int row = visible_idx / cols;

    int x = start_x + (col * (CARD_WIDTH + PADDING));
    int y = start_y + (row * (CARD_HEIGHT + PADDING)) - scroll_offset;

    // Culling
    if (y + CARD_HEIGHT < TOP_BAR_HEIGHT || y > screen_height) {
      visible_idx++;
      continue;
    }

    Rectangle card_rect = {x, y, CARD_WIDTH, CARD_HEIGHT};
    bool hovered =
        CheckCollisionPointRec(GetMousePosition(), card_rect) && !show_modal;

    if (hovered) {
      DrawRectangleRec(
          (Rectangle){x - 4, y - 4, CARD_WIDTH + 8, CARD_HEIGHT + 8},
          Fade(SKYBLUE, 0.3f));
      DrawRectangleLinesEx(
          (Rectangle){x - 2, y - 2, CARD_WIDTH + 4, CARD_HEIGHT + 4}, 2,
          SKYBLUE);
    } else {
      DrawRectangleRec(card_rect, GetColor(0x2a2a2aff));
    }

    // Texture
    if (i < current_texture_count && current_textures[i].id != 0) {
      DrawTexturePro(current_textures[i],
                     (Rectangle){0, 0, current_textures[i].width,
                                 current_textures[i].height},
                     card_rect, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
      DrawText(active_list->apps[i].name, x + 10, y + 50, 20, WHITE);
    }

    if (hovered) {
      DrawRectangle(x, y, CARD_WIDTH, CARD_HEIGHT, Fade(BLACK, 0.7f));
      DrawText("LAUNCH", x + CARD_WIDTH / 2 - 40, y + CARD_HEIGHT / 2 - 20, 20,
               GREEN);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          !CheckCollisionPointRec(
              GetMousePosition(),
              (Rectangle){x + CARD_WIDTH - 60, y + 5, 55, 25})) {
        PlaySound(snd_launch);
        if (active_list->apps[i].steam_id != -1) {
          char url[64];
          snprintf(url, sizeof(url), "steam://run/%d",
                   active_list->apps[i].steam_id);
          OpenURL(url);
        } else {
          char cmd[1100];
          snprintf(cmd, sizeof(cmd), "%s &", active_list->apps[i].exec);
          system(cmd);
        }
      }

      // Edit Button
      DrawRectangle(x + CARD_WIDTH - 60, y + 5, 55, 25, Fade(WHITE, 0.2f));
      DrawText("EDIT", x + CARD_WIDTH - 55, y + 10, 10, WHITE);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointRec(
              GetMousePosition(),
              (Rectangle){x + CARD_WIDTH - 60, y + 5, 55, 25})) {
        show_modal = true;
        is_edit_mode = true;
        app_to_edit = &active_list->apps[i];
        strncpy(input_name, app_to_edit->name, 255);
        strncpy(input_path, app_to_edit->exec, 1023);
        strncpy(input_img, app_to_edit->image_path, 1023);
      }

      DrawRectangle(x, y + CARD_HEIGHT - 40, CARD_WIDTH, 40, Fade(BLACK, 0.9f));
      BeginScissorMode(x, y + CARD_HEIGHT - 40, CARD_WIDTH, 40);
      DrawText(active_list->apps[i].name, x + 5, y + CARD_HEIGHT - 30, 20,
               WHITE);
      EndScissorMode();
    } else if (i < current_texture_count && current_textures[i].id == 0) {
      DrawRectangle(x, y + CARD_HEIGHT - 40, CARD_WIDTH, 40, Fade(BLACK, 0.8f));
      BeginScissorMode(x, y + CARD_HEIGHT - 40, CARD_WIDTH, 40);
      DrawText(active_list->apps[i].name, x + 5, y + CARD_HEIGHT - 30, 20,
               WHITE);
      EndScissorMode();
    }

    visible_idx++;
  }

  EndScissorMode();

  if (show_modal)
    DrawnModal(screen_width, screen_height);
}
