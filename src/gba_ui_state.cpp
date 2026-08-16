#include "gba_ui_state.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <utility>

namespace fs = std::filesystem;

GbaUiStateStore::GbaUiStateStore(std::string path) : path_(std::move(path)) {}

bool GbaUiStateStore::Load(GbaUiState *state) const {
  if (!state) return false;
  std::ifstream input(fs::u8path(path_));
  int version = 0;
  int settings_open = 0;
  GbaUiState loaded;
  if (!(input >> version >> loaded.active_tab >> loaded.scroll_row >> settings_open >>
        loaded.settings_selected >> std::quoted(loaded.selected_game_id)) || version != 1) {
    return false;
  }
  loaded.settings_open = settings_open != 0;
  *state = std::move(loaded);
  return true;
}

bool GbaUiStateStore::Save(const GbaUiState &state) const {
  const fs::path path = fs::u8path(path_);
  std::error_code error;
  fs::create_directories(path.parent_path(), error);
  const fs::path temporary = path.string() + ".tmp";
  {
    std::ofstream output(temporary, std::ios::trunc);
    if (!output) return false;
    output << 1 << ' ' << state.active_tab << ' ' << state.scroll_row << ' '
           << (state.settings_open ? 1 : 0) << ' ' << state.settings_selected << ' '
           << std::quoted(state.selected_game_id) << '\n';
    if (!output) return false;
  }
  fs::rename(temporary, path, error);
  if (!error) return true;
  fs::remove(path, error);
  error.clear();
  fs::rename(temporary, path, error);
  return !error;
}
