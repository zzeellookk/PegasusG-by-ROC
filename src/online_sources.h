#pragma once

#include <string>
#include <vector>

#include "ui_types.h"

struct OnlineBookSource {
  std::string id;
  std::string name;
  std::string url;
};

class OnlineSourceConfig {
 public:
  static OnlineSourceConfig LoadFromIni(const std::string &path);
  static OnlineSourceConfig Defaults();
  bool SaveToIni(const std::string &path) const;

  bool Empty() const;
  int SourceCount() const;
  const std::vector<OnlineBookSource> &Sources() const;
  const OnlineBookSource &SelectedSource() const;
  int SelectedIndex() const;
  void SelectIndex(int index);

 private:
  std::vector<OnlineBookSource> sources_;
  int selected_index_ = 0;
};

std::vector<LibraryItem> MakeOnlineNovelItems(const ModuleDefinition &module,
                                              const OnlineBookSource &source);
std::vector<LibraryItem> MakeOnlineSourceSettingsItems(
    const ModuleDefinition &module, const OnlineSourceConfig &config);
