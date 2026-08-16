#pragma once

#include <string>
#include <vector>

#include "ui_types.h"

enum class LibraryFilter {
  All = 0,
  Recent = 1,
  Favorites = 2,
};

LibraryFilter LibraryFilterFromIndex(int index);

class LibraryCatalog {
 public:
  LibraryCatalog() = default;
  explicit LibraryCatalog(std::vector<std::vector<LibraryItem>> modules);

  static LibraryCatalog Demo(const std::vector<ModuleDefinition> &modules);

  int ModuleCount() const;
  bool Empty() const;
  size_t TotalItemCount() const;
  const std::vector<LibraryItem> &ItemsForModule(int module_index) const;
  const LibraryItem *Item(int module_index, int item_index) const;

  std::vector<int> Query(int module_index, LibraryFilter filter,
                         const std::string &search_query = std::string()) const;
  int Count(int module_index, LibraryFilter filter,
            const std::string &search_query = std::string()) const;

  void SetItemsForModule(int module_index, std::vector<LibraryItem> items);
  bool SetFavorite(int module_index, int item_index, bool favorite);
  bool ToggleFavorite(int module_index, int item_index);
  bool MarkOpened(int module_index, int item_index);

 private:
  std::vector<std::vector<LibraryItem>> modules_;
  int next_recent_order_ = 1;
};
