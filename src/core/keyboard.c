#include "keyboard.h"

void update_keyboard(w_keyboard *kb) {
  kb->left = IsKeyDown(KEY_LEFT);
  kb->right = IsKeyDown(KEY_RIGHT);
  kb->jump = IsKeyDown(KEY_SPACE);
  kb->inventory = IsKeyDown(KEY_RIGHT_ALT);
}

void clear_keyboard(w_keyboard *kb) { kb->key = 0; }
