#include "online_sources.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace {

std::string Trim(const std::string &value) {
  size_t first = 0;
  while (first < value.size() &&
         std::isspace(static_cast<unsigned char>(value[first]))) {
    ++first;
  }
  size_t last = value.size();
  while (last > first &&
         std::isspace(static_cast<unsigned char>(value[last - 1]))) {
    --last;
  }
  return value.substr(first, last - first);
}

std::string LowerAscii(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

unsigned char ClampColor(int value) {
  return static_cast<unsigned char>(std::max(0, std::min(value, 255)));
}

LibraryItem MakeSourceItem(const ModuleDefinition &module,
                           const OnlineBookSource &source, int index,
                           bool selected) {
  LibraryItem item;
  item.id = "settings:online-source:" + source.id;
  item.module_id = module.id;
  item.title = selected ? "当前书源 · " + source.name : "书源 · " + source.name;
  item.metadata = source.url;
  item.path = source.url;
  item.source_id = source.id;
  item.source_url = source.url;
  item.favorite = selected;
  item.red = ClampColor(229 + index * 5);
  item.green = ClampColor(242 - index * 3);
  item.blue = ClampColor(255 - index * 4);
  return item;
}

}  // namespace

OnlineSourceConfig OnlineSourceConfig::Defaults() {
  OnlineSourceConfig config;
  config.sources_ = {
      {"demo", "示例在线书库", "https://example.invalid/roc-novels"},
      {"lan", "局域网书库", "http://192.168.1.2:8080/books"},
  };
  config.selected_index_ = 0;
  return config;
}

OnlineSourceConfig OnlineSourceConfig::LoadFromIni(const std::string &path) {
  std::ifstream input(path);
  if (!input) return Defaults();

  std::string selected_id;
  std::string current_section;
  std::vector<std::string> order;
  std::unordered_map<std::string, OnlineBookSource> parsed;

  std::string line;
  while (std::getline(input, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == '#' || line[0] == ';') continue;
    if (line.front() == '[' && line.back() == ']') {
      current_section = Trim(line.substr(1, line.size() - 2));
      if (!current_section.empty() && LowerAscii(current_section) != "online") {
        if (!parsed.count(current_section)) {
          OnlineBookSource source;
          source.id = current_section;
          parsed[current_section] = source;
          order.push_back(current_section);
        }
      }
      continue;
    }

    const size_t equals = line.find('=');
    if (equals == std::string::npos) continue;
    const std::string key = LowerAscii(Trim(line.substr(0, equals)));
    const std::string value = Trim(line.substr(equals + 1));
    if (key.empty()) continue;

    if (LowerAscii(current_section) == "online" || current_section.empty()) {
      if (key == "selected" || key == "current") selected_id = value;
      continue;
    }

    OnlineBookSource &source = parsed[current_section];
    if (key == "id") {
      source.id = value.empty() ? current_section : value;
    } else if (key == "name") {
      source.name = value;
    } else if (key == "url") {
      source.url = value;
    }
  }

  OnlineSourceConfig config;
  for (const std::string &section : order) {
    OnlineBookSource source = parsed[section];
    if (source.id.empty()) source.id = section;
    if (source.name.empty()) source.name = source.id;
    if (source.url.empty()) continue;
    config.sources_.push_back(std::move(source));
  }
  if (config.sources_.empty()) return Defaults();

  config.selected_index_ = 0;
  if (!selected_id.empty()) {
    for (int i = 0; i < static_cast<int>(config.sources_.size()); ++i) {
      if (config.sources_[i].id == selected_id) {
        config.selected_index_ = i;
        break;
      }
    }
  }
  return config;
}

bool OnlineSourceConfig::SaveToIni(const std::string &path) const {
  if (path.empty()) return false;
  std::error_code error;
  const std::filesystem::path file(path);
  std::filesystem::create_directories(file.parent_path(), error);
  std::ofstream output(path, std::ios::trunc);
  if (!output) return false;
  output << "[online]\n";
  output << "selected=" << SelectedSource().id << "\n\n";
  for (const auto &source : sources_) {
    output << "[" << source.id << "]\n";
    output << "name=" << source.name << "\n";
    output << "url=" << source.url << "\n\n";
  }
  return true;
}

bool OnlineSourceConfig::Empty() const {
  return sources_.empty();
}

int OnlineSourceConfig::SourceCount() const {
  return static_cast<int>(sources_.size());
}

const std::vector<OnlineBookSource> &OnlineSourceConfig::Sources() const {
  return sources_;
}

const OnlineBookSource &OnlineSourceConfig::SelectedSource() const {
  if (sources_.empty()) {
    static const OnlineBookSource kEmpty{"", "未配置书源", ""};
    return kEmpty;
  }
  return sources_[std::max(0, std::min(selected_index_, SourceCount() - 1))];
}

int OnlineSourceConfig::SelectedIndex() const {
  return selected_index_;
}

void OnlineSourceConfig::SelectIndex(int index) {
  if (sources_.empty()) {
    selected_index_ = 0;
  } else {
    selected_index_ = std::max(0, std::min(index, SourceCount() - 1));
  }
}

std::vector<LibraryItem> MakeOnlineNovelItems(const ModuleDefinition &module,
                                              const OnlineBookSource &source) {
  static const char *kNames[] = {
      "云端书架", "新书速递", "继续阅读", "排行榜", "短篇精选", "完本收藏",
      "本周更新", "离线候选", "作者专题", "搜索结果", "最近同步", "书单入口",
  };
  static const unsigned char kPastels[][3] = {
      {255, 228, 236}, {229, 242, 255}, {234, 245, 232}, {255, 242, 218},
      {238, 232, 255}, {226, 247, 244}, {255, 232, 218}, {236, 239, 245},
  };

  std::vector<LibraryItem> items;
  items.reserve(12);
  for (int i = 0; i < 12; ++i) {
    const auto &color = kPastels[(i + 1) % 8];
    LibraryItem item;
    item.id = "online:" + source.id + ":" + std::to_string(i);
    item.module_id = module.id;
    item.title = "在线 · " + std::string(kNames[i]);
    item.metadata = source.name;
    item.path = source.url + "/book/" + std::to_string(i);
    item.source_id = source.id;
    item.source_url = source.url;
    item.favorite = i == 1 || i == 5;
    item.recent_order = i < 3 ? 50 - i : 0;
    item.progress_percent = -1;
    item.red = color[0];
    item.green = color[1];
    item.blue = color[2];
    items.push_back(std::move(item));
  }
  return items;
}

std::vector<LibraryItem> MakeOnlineSourceSettingsItems(
    const ModuleDefinition &module, const OnlineSourceConfig &config) {
  std::vector<LibraryItem> items;
  const auto &sources = config.Sources();
  items.reserve(sources.size());
  for (int i = 0; i < static_cast<int>(sources.size()); ++i) {
    items.push_back(MakeSourceItem(module, sources[i], i, i == config.SelectedIndex()));
  }
  return items;
}
