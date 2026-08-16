#pragma once

#include <string>
#include <vector>

#include "library_catalog.h"

class CoverGridDataSource {
 public:
  void Refresh(const LibraryCatalog &catalog, int module_index, LibraryFilter filter,
               const std::string &query);

  int Count() const;
  int LibraryIndexAt(int visible_index) const;
  const LibraryItem *ItemAt(const LibraryCatalog &catalog, int visible_index) const;

 private:
  int module_index_ = 0;
  std::vector<int> visible_indices_;
};

