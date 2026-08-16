#include "optimized_image_path.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

ResolvedImagePath ResolveOptimizedImagePath(const std::string &source_path,
                                            bool use_mini_assets) {
  if (source_path.empty()) return {};
  if (!use_mini_assets) return {source_path, false};

  const fs::path source = fs::u8path(source_path);
  const fs::path directory = source.parent_path();
  std::string stem = source.stem().u8string();
  std::transform(stem.begin(), stem.end(), stem.begin(),
                 [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
  std::string mini_stem;
  if (stem == "boxfront") mini_stem = "mini_boxfront";
  else if (stem == "logo" || stem == "lgoo") mini_stem = "mini_logo";
  else return {source_path, false};

  std::string source_extension = source.extension().u8string();
  std::transform(source_extension.begin(), source_extension.end(), source_extension.begin(),
                 [](unsigned char value) { return static_cast<char>(std::tolower(value)); });

  std::vector<std::string> suffixes;
  if (source_extension == ".jpg") {
    suffixes = {".jpg", ".png"};
  } else {
    suffixes = {".png", ".jpg"};
  }

  for (const std::string &suffix : suffixes) {
    const fs::path candidate = directory / fs::u8path(mini_stem + suffix);
    std::error_code error;
    if (fs::is_regular_file(candidate, error)) {
      return {candidate.u8string(), true};
    }
  }
  return {source_path, false};
}
