#include "online_sources.h"

#include <cassert>
#include <filesystem>
#include <fstream>

int main() {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "roc_online_sources_test.ini";
  {
    std::ofstream out(path);
    out << "[online]\n";
    out << "selected=lan\n\n";
    out << "[demo]\n";
    out << "name=示例在线书库\n";
    out << "url=https://example.invalid/roc-novels\n\n";
    out << "[lan]\n";
    out << "name=局域网书库\n";
    out << "url=http://192.168.1.2:8080/books\n";
  }

  OnlineSourceConfig config = OnlineSourceConfig::LoadFromIni(path.u8string());
  assert(config.SourceCount() == 2);
  assert(config.SelectedIndex() == 1);
  assert(config.SelectedSource().id == "lan");
  assert(config.SelectedSource().url == "http://192.168.1.2:8080/books");

  config.SelectIndex(0);
  assert(config.SaveToIni(path.u8string()));
  OnlineSourceConfig saved = OnlineSourceConfig::LoadFromIni(path.u8string());
  assert(saved.SelectedIndex() == 0);
  assert(saved.SelectedSource().id == "demo");

  const ModuleDefinition novel{"novel", "小说", "小说库", ModuleIcon::Novel};
  const auto online_items = MakeOnlineNovelItems(novel, saved.SelectedSource());
  assert(online_items.size() == 12);
  assert(online_items[0].source_id == "demo");
  assert(!online_items[0].source_url.empty());

  const ModuleDefinition settings{"settings", "设置", "系统与前端设置", ModuleIcon::Settings};
  const auto settings_items = MakeOnlineSourceSettingsItems(settings, saved);
  assert(settings_items.size() == 2);
  assert(settings_items[0].favorite);
  assert(!settings_items[1].favorite);

  std::filesystem::remove(path);
  return 0;
}
