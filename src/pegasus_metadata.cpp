#include "pegasus_metadata.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace fs = std::filesystem;

namespace {

struct ParsedEntry {
  std::string title;
  std::string developer;
  std::string description;
  std::string sort_key;
  std::vector<std::string> files;
  std::string cover;
  std::string logo;
  std::string video;
  GbaCore core = GbaCore::Mgba;
  bool has_core = false;
};

struct ParsedMetadata {
  std::string collection;
  std::vector<ParsedEntry> entries;
};

std::string Trim(std::string value) {
  const auto blank = [](unsigned char ch) { return std::isspace(ch) != 0; };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                          [&](char ch) { return !blank(ch); }));
  value.erase(std::find_if(value.rbegin(), value.rend(),
                           [&](char ch) { return !blank(ch); }).base(), value.end());
  return value;
}

std::string LowerAscii(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

bool IsRomExtension(const fs::path &path) {
  const std::string ext = LowerAscii(path.extension().u8string());
  return ext == ".gba" || ext == ".zip" || ext == ".7z";
}

std::string StableId(const std::string &text) {
  std::uint64_t value = 1469598103934665603ULL;
  for (unsigned char ch : text) {
    value ^= ch;
    value *= 1099511628211ULL;
  }
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

std::string UnescapeDescription(std::string value) {
  std::string result;
  result.reserve(value.size());
  for (size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '\\' && i + 1 < value.size() && value[i + 1] == 'n') {
      result.push_back('\n');
      ++i;
    } else {
      result.push_back(value[i]);
    }
  }
  return result;
}

ParsedMetadata ParseMetadata(const fs::path &metadata) {
  std::ifstream input(metadata, std::ios::binary);
  ParsedMetadata parsed;
  ParsedEntry current;
  bool has_current = false;
  bool reading_files = false;
  bool reading_launch = false;
  std::string line;

  auto apply_core_hint = [](ParsedEntry *entry, const std::string &text) {
    const std::string lower = LowerAscii(text);
    if (lower.find("vba_next") != std::string::npos ||
        lower.find("vba-next") != std::string::npos) {
      entry->core = GbaCore::VbaNext;
      entry->has_core = true;
    } else if (lower.find("vbam") != std::string::npos ||
               lower.find("vba-m") != std::string::npos) {
      entry->core = GbaCore::Vbam;
      entry->has_core = true;
    } else if (lower.find("gpsp") != std::string::npos) {
      entry->core = GbaCore::Gpsp;
      entry->has_core = true;
    } else if (lower.find("mgba") != std::string::npos) {
      entry->core = GbaCore::Mgba;
      entry->has_core = true;
    }
  };

  auto finish = [&]() {
    if (has_current && !current.title.empty()) parsed.entries.push_back(std::move(current));
    current = ParsedEntry{};
    has_current = false;
    reading_files = false;
    reading_launch = false;
  };

  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!has_current && line.rfind("collection:", 0) == 0) {
      parsed.collection = Trim(line.substr(11));
      continue;
    }
    if (line.rfind("game:", 0) == 0) {
      finish();
      current.title = Trim(line.substr(5));
      has_current = true;
      continue;
    }
    if (!has_current) continue;
    if (reading_files && line.rfind("  ", 0) == 0) {
      const std::string file = Trim(line);
      if (!file.empty()) current.files.push_back(file);
      continue;
    }
    if (reading_launch && line.rfind("  ", 0) == 0) {
      apply_core_hint(&current, line);
      continue;
    }
    reading_files = false;
    reading_launch = false;
    const size_t colon = line.find(':');
    if (colon == std::string::npos) continue;
    const std::string key = Trim(line.substr(0, colon));
    const std::string value = Trim(line.substr(colon + 1));
    if (key == "file") current.files.push_back(value);
    else if (key == "files") reading_files = true;
    else if (key == "launch") {
      reading_launch = true;
      apply_core_hint(&current, value);
    } else if (key == "sort-by") current.sort_key = value;
    else if (key == "developer") current.developer = value;
    else if (key == "description") current.description = UnescapeDescription(value);
    else if (key == "assets.box_front") current.cover = value;
    else if (key == "assets.logo") current.logo = value;
    else if (key == "assets.video") current.video = value;
  }
  finish();
  return parsed;
}

std::string FindRegularFile(const fs::path &base,
                            const std::vector<std::string> &candidates) {
  for (const std::string &candidate : candidates) {
    if (candidate.empty()) continue;
    const fs::path path = fs::u8path(candidate).is_absolute()
                              ? fs::u8path(candidate)
                              : base / fs::u8path(candidate);
    std::error_code error;
    if (fs::is_regular_file(path, error)) return path.u8string();
  }
  return {};
}

std::string AssetFor(const fs::path &base, const ParsedEntry &entry,
                     const std::string &explicit_path,
                     const std::vector<std::string> &names) {
  if (!explicit_path.empty()) {
    const std::string found = FindRegularFile(base, {explicit_path});
    if (!found.empty()) return found;
  }
  std::vector<std::string> candidates;
  candidates.reserve(names.size());
  for (const std::string &name : names) {
    candidates.push_back((fs::path("media") / fs::u8path(entry.title) / name).u8string());
  }
  return FindRegularFile(base, candidates);
}

std::unordered_set<std::string> LoadOverrides(const std::string &path) {
  std::unordered_set<std::string> result;
  if (path.empty()) return result;
  std::ifstream input(fs::u8path(path));
  std::string line;
  while (std::getline(input, line)) {
    line = Trim(line);
    if (!line.empty() && line[0] != '#') result.insert(line);
  }
  return result;
}

void FindMetadataFiles(const fs::path &root, std::vector<fs::path> *result) {
  std::error_code error;
  if (fs::is_regular_file(root, error) && root.filename() == "metadata.pegasus.txt") {
    result->push_back(root);
    return;
  }
  if (!fs::is_directory(root, error)) return;
  fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                      error);
  const fs::recursive_directory_iterator end;
  for (; !error && it != end; it.increment(error)) {
    if (it->is_directory(error) && it->path().filename() == "media") {
      it.disable_recursion_pending();
      continue;
    }
    if (it->is_regular_file(error) && it->path().filename() == "metadata.pegasus.txt") {
      result->push_back(it->path());
    }
  }
}

enum class CollectionKind {
  Unknown,
  Standard,
  Mod,
  Rumble,
};

CollectionKind ClassifyCollection(const std::string &collection, const fs::path &directory) {
  const std::string collection_name = LowerAscii(Trim(collection));
  const std::string folder_name = LowerAscii(directory.filename().u8string());
  const std::string text = collection_name + " " + folder_name;
  if (text.find("gba vib") != std::string::npos ||
      text.find("rumble") != std::string::npos ||
      text.find("vibration") != std::string::npos ||
      text.find("震动") != std::string::npos || text.find("振动") != std::string::npos ||
      text.find("震動") != std::string::npos) {
    return CollectionKind::Rumble;
  }
  if (text.find("gba hack") != std::string::npos ||
      text.find("romhack") != std::string::npos || text.find("改版") != std::string::npos) {
    return CollectionKind::Mod;
  }
  if (collection_name == "gba" || folder_name == "gba") return CollectionKind::Standard;
  return CollectionKind::Unknown;
}

}  // namespace

bool LooksLikeGbaMod(const GbaGame &game) {
  const std::string text = LowerAscii(game.title + " " + game.developer + " " + game.rom_path);
  static const std::vector<std::string> kMarkers = {
      "改版", "替换版", "重制版", "魔改", "hack", "romhack", "究极绿宝石",
      "漆黑的魅影", "东方人形剧", "去吧！皮卡丘", "去吧！伊布", "剑盾v",
      "彩色鞭", "随机版", "增强版",
  };
  return std::any_of(kMarkers.begin(), kMarkers.end(),
                     [&](const std::string &marker) { return text.find(marker) != std::string::npos; });
}

bool LooksLikeGbaRumble(const GbaGame &game) {
  const std::string text = LowerAscii(game.title + " " + game.developer + " " + game.rom_path);
  static const std::vector<std::string> kMarkers = {
      "震动", "振动", "震動", "rumble", "vibration",
  };
  return std::any_of(kMarkers.begin(), kMarkers.end(),
                     [&](const std::string &marker) { return text.find(marker) != std::string::npos; });
}

PegasusScanReport ScanPegasusGbaRoots(const std::vector<std::string> &roots,
                                      const std::string &mod_overrides_path) {
  PegasusScanReport report;
  std::vector<fs::path> metadata_files;
  for (const std::string &root : roots) FindMetadataFiles(fs::u8path(root), &metadata_files);
  std::sort(metadata_files.begin(), metadata_files.end());
  metadata_files.erase(std::unique(metadata_files.begin(), metadata_files.end()),
                       metadata_files.end());
  report.metadata_files = static_cast<int>(metadata_files.size());

  const auto overrides = LoadOverrides(mod_overrides_path);
  std::unordered_set<std::string> seen_roms;
  std::unordered_map<std::string, std::unordered_set<std::string>> referenced_by_directory;

  for (const fs::path &metadata : metadata_files) {
    const fs::path base = metadata.parent_path();
    const ParsedMetadata parsed = ParseMetadata(metadata);
    const CollectionKind collection_kind = ClassifyCollection(parsed.collection, base);
    report.parsed_entries += static_cast<int>(parsed.entries.size());
    for (const ParsedEntry &entry : parsed.entries) {
      std::vector<std::string> roms;
      for (const std::string &candidate : entry.files) {
        const std::string found = FindRegularFile(base, {candidate});
        if (!found.empty()) roms.push_back(found);
      }
      if (roms.empty()) {
        ++report.missing_rom_entries;
        continue;
      }
      std::error_code canonical_error;
      const std::string canonical = fs::weakly_canonical(fs::u8path(roms.front()),
                                                          canonical_error).u8string();
      const std::string dedupe_key = canonical_error ? roms.front() : canonical;
      if (!seen_roms.insert(dedupe_key).second) {
        ++report.duplicate_rom_entries;
        continue;
      }

      GbaGame game;
      game.id = StableId(dedupe_key);
      game.title = entry.title;
      game.developer = entry.developer;
      game.description = entry.description;
      game.sort_key = entry.sort_key;
      game.rom_path = roms.front();
      if (roms.size() > 1) game.alternate_roms.assign(roms.begin() + 1, roms.end());
      game.cover_path = AssetFor(base, entry, entry.cover,
                                 {"boxfront.png", "boxFront.png", "cover.png", "boxfront.jpg"});
      game.logo_path = AssetFor(base, entry, entry.logo, {"logo.png", "logo.jpg"});
      game.video_path = AssetFor(base, entry, entry.video, {"video.mp4", "video.mkv"});
      game.metadata_path = metadata.u8string();
      if (collection_kind == CollectionKind::Rumble) {
        game.is_rumble = true;
      } else if (collection_kind == CollectionKind::Mod) {
        game.is_mod = true;
      } else if (collection_kind == CollectionKind::Unknown) {
        game.is_rumble = LooksLikeGbaRumble(game);
        game.is_mod = !game.is_rumble &&
            (overrides.count(game.title) != 0 || LooksLikeGbaMod(game));
      }
      if (entry.has_core) {
        game.default_core = entry.core;
      } else {
        game.default_core = game.is_rumble ? GbaCore::Gpsp : GbaCore::Mgba;
      }
      game.core = game.default_core;
      report.games.push_back(std::move(game));

      auto &referenced = referenced_by_directory[base.u8string()];
      for (const std::string &rom : roms) {
        referenced.insert(fs::u8path(rom).filename().u8string());
      }
    }
  }

  for (const auto &[directory_text, referenced] : referenced_by_directory) {
    std::error_code error;
    for (const auto &entry : fs::directory_iterator(fs::u8path(directory_text), error)) {
      if (!entry.is_regular_file(error) || !IsRomExtension(entry.path())) continue;
      if (!referenced.count(entry.path().filename().u8string())) ++report.unreferenced_roms;
    }
  }

  std::stable_sort(report.games.begin(), report.games.end(), [](const GbaGame &left,
                                                                 const GbaGame &right) {
    if (left.sort_key != right.sort_key) return left.sort_key < right.sort_key;
    return left.title < right.title;
  });
  if (report.metadata_files == 0) report.warnings.push_back("No metadata.pegasus.txt found");
  return report;
}
