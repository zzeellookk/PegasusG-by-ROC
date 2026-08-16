#pragma once

#include <string>

struct GbaUiState {
  int active_tab = 1;
  std::string selected_game_id;
  int scroll_row = 0;
  bool settings_open = false;
  int settings_selected = 0;
};

class GbaUiStateStore {
 public:
  explicit GbaUiStateStore(std::string path);

  bool Load(GbaUiState *state) const;
  bool Save(const GbaUiState &state) const;

 private:
  std::string path_;
};
