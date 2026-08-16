#pragma once

#include <string>

struct ResolvedImagePath {
  std::string path;
  bool optimized = false;
};

ResolvedImagePath ResolveOptimizedImagePath(const std::string &source_path,
                                            bool use_mini_assets);
