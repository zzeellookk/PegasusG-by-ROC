#include "cover_grid_data_source.h"

void CoverGridDataSource::Refresh(const LibraryCatalog &catalog, int module_index,
                                  LibraryFilter filter, const std::string &query) {
  module_index_ = module_index;
  visible_indices_ = catalog.Query(module_index, filter, query);
}

int CoverGridDataSource::Count() const {
  return static_cast<int>(visible_indices_.size());
}

int CoverGridDataSource::LibraryIndexAt(int visible_index) const {
  if (visible_index < 0 || visible_index >= Count()) return -1;
  return visible_indices_[visible_index];
}

const LibraryItem *CoverGridDataSource::ItemAt(const LibraryCatalog &catalog,
                                               int visible_index) const {
  return catalog.Item(module_index_, LibraryIndexAt(visible_index));
}

