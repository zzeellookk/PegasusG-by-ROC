#include "gba_ui_state.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main() {
  const fs::path root = fs::temp_directory_path() / "roc_gba_ui_state_test";
  const fs::path path = root / "ui_state.txt";
  fs::remove_all(root);

  GbaUiStateStore store(path.u8string());
  GbaUiState missing;
  assert(!store.Load(&missing));

  GbaUiState saved;
  saved.active_tab = 3;
  saved.selected_game_id = "game id with spaces and \"quotes\"";
  saved.scroll_row = 7;
  saved.settings_open = true;
  saved.settings_selected = 2;
  assert(store.Save(saved));

  GbaUiState loaded;
  assert(store.Load(&loaded));
  assert(loaded.active_tab == saved.active_tab);
  assert(loaded.selected_game_id == saved.selected_game_id);
  assert(loaded.scroll_row == saved.scroll_row);
  assert(loaded.settings_open == saved.settings_open);
  assert(loaded.settings_selected == saved.settings_selected);

  std::ofstream(path, std::ios::trunc) << "invalid\n";
  assert(!store.Load(&loaded));
  fs::remove_all(root);
  return 0;
}
