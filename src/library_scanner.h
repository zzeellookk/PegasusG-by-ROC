#pragma once

#include <atomic>
#include <string>
#include <vector>

#include "ui_types.h"

struct LibraryScanOptions {
  std::vector<std::string> roots;
  int max_items = 10000;
};

std::vector<std::string> SplitLibraryRootList(const std::string &value);

std::vector<std::vector<LibraryItem>> ScanLibraryRoots(
    const std::vector<ModuleDefinition> &modules, const LibraryScanOptions &options,
    const std::atomic<bool> *cancel = nullptr);

