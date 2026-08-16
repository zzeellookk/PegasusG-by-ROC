#include "library_scanner.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {

std::string LowerAscii(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

std::string ExtensionOf(const fs::path &path) {
  return LowerAscii(path.extension().u8string());
}

std::string TitleFromPath(const fs::path &path, bool keep_extension = false) {
  return keep_extension ? path.filename().u8string() : path.stem().u8string();
}

int ModuleIndex(const std::unordered_map<std::string, int> &indices, const char *id) {
  const auto found = indices.find(id);
  return found == indices.end() ? -1 : found->second;
}

bool HasFile(const fs::path &directory, const char *name) {
  std::error_code error;
  return fs::is_regular_file(directory / name, error);
}

bool HasAnyXp3(const fs::path &directory) {
  std::error_code error;
  fs::directory_iterator it(directory, fs::directory_options::skip_permission_denied, error);
  fs::directory_iterator end;
  for (; !error && it != end; it.increment(error)) {
    if (it->is_regular_file(error) && ExtensionOf(it->path()) == ".xp3") return true;
  }
  return false;
}

const char *ModuleForFile(const fs::path &path) {
  static const std::unordered_set<std::string> kNovel{
      ".txt", ".epub", ".pdf", ".mobi", ".azw3"};
  static const std::unordered_set<std::string> kComic{
      ".cbz", ".cbr", ".cb7", ".zip", ".rar", ".7z"};
  static const std::unordered_set<std::string> kVideo{
      ".mp4", ".mkv", ".avi", ".mov", ".webm", ".flv", ".m4v"};
  static const std::unordered_set<std::string> kMusic{
      ".mp3", ".flac", ".ogg", ".wav", ".m4a", ".aac", ".opus"};
  const std::string ext = ExtensionOf(path);
  if (kNovel.count(ext)) return "novel";
  if (kComic.count(ext)) return "comic";
  if (kVideo.count(ext)) return "video";
  if (kMusic.count(ext)) return "music";
  return "";
}

const char *ModuleForDirectory(const fs::path &path) {
  if (HasFile(path, "nscript.dat") || HasFile(path, "0.txt")) return "ons";
  if (HasFile(path, "data.xp3") || HasAnyXp3(path)) return "krkr";
  return "";
}

LibraryItem MakeItem(const ModuleDefinition &module, const fs::path &path, int color_index,
                     bool directory) {
  static const unsigned char kPastels[][3] = {
      {255, 228, 236}, {229, 242, 255}, {234, 245, 232}, {255, 242, 218},
      {238, 232, 255}, {226, 247, 244}, {255, 232, 218}, {236, 239, 245},
  };
  const auto &color = kPastels[color_index % 8];
  LibraryItem item;
  item.module_id = module.id;
  item.path = path.u8string();
  item.id = module.id + ":" + item.path;
  item.title = module.title + " · " + TitleFromPath(path, directory);
  item.metadata = "已扫描";
  item.red = color[0];
  item.green = color[1];
  item.blue = color[2];
  return item;
}

}  // namespace

std::vector<std::string> SplitLibraryRootList(const std::string &value) {
  std::vector<std::string> roots;
  std::string current;
  for (char ch : value) {
    const bool separator = ch == ';' || ch == '\n' || ch == '\r' || ch == '\t' ||
#ifdef _WIN32
                           ch == '|';
#else
                           ch == ':';
#endif
    if (separator) {
      if (!current.empty()) roots.push_back(current);
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  if (!current.empty()) roots.push_back(current);
  return roots;
}

std::vector<std::vector<LibraryItem>> ScanLibraryRoots(
    const std::vector<ModuleDefinition> &modules, const LibraryScanOptions &options,
    const std::atomic<bool> *cancel) {
  std::vector<std::vector<LibraryItem>> result(modules.size());
  std::unordered_map<std::string, int> module_indices;
  for (int i = 0; i < static_cast<int>(modules.size()); ++i) {
    module_indices[modules[i].id] = i;
  }

  int item_count = 0;
  for (const std::string &root_text : options.roots) {
    if (cancel && cancel->load()) break;
    std::error_code error;
    fs::path root = fs::u8path(root_text);
    if (!fs::exists(root, error)) continue;

    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied, error);
    fs::recursive_directory_iterator end;
    for (; !error && it != end; it.increment(error)) {
      if (cancel && cancel->load()) break;
      if (item_count >= options.max_items) break;

      const fs::path path = it->path();
      std::error_code entry_error;
      const bool is_directory = it->is_directory(entry_error);
      const bool is_file = !is_directory && it->is_regular_file(entry_error);
      const char *module_id = "";
      if (is_directory) {
        module_id = ModuleForDirectory(path);
        if (*module_id) it.disable_recursion_pending();
      } else if (is_file) {
        module_id = ModuleForFile(path);
      }
      if (!*module_id) continue;

      const int module_index = ModuleIndex(module_indices, module_id);
      if (module_index < 0) continue;
      result[module_index].push_back(
          MakeItem(modules[module_index], path, item_count + module_index, is_directory));
      ++item_count;
    }
  }

  for (auto &items : result) {
    std::sort(items.begin(), items.end(), [](const LibraryItem &left, const LibraryItem &right) {
      return left.title < right.title;
    });
  }
  return result;
}

