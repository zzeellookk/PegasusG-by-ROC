#include "cover_grid_data_source.h"

#include <cassert>
#include <vector>

int main() {
  const std::vector<ModuleDefinition> modules = {
      {"novel", "小说", "小说库", ModuleIcon::Novel},
      {"comic", "漫画", "漫画库", ModuleIcon::Comic},
  };
  LibraryCatalog catalog = LibraryCatalog::Demo(modules);

  CoverGridDataSource grid;
  grid.Refresh(catalog, 0, LibraryFilter::All, "");
  assert(grid.Count() == 16);
  assert(grid.ItemAt(catalog, 0) != nullptr);

  grid.Refresh(catalog, 0, LibraryFilter::Recent, "");
  assert(grid.Count() == 4);

  grid.Refresh(catalog, 0, LibraryFilter::Favorites, "");
  const int favorites_before = grid.Count();
  assert(favorites_before > 0);

  const int first_visible = grid.LibraryIndexAt(0);
  assert(first_visible >= 0);
  catalog.SetFavorite(0, first_visible, false);
  grid.Refresh(catalog, 0, LibraryFilter::Favorites, "");
  assert(grid.Count() == favorites_before - 1);
  catalog.SetFavorite(0, first_visible, true);
  grid.Refresh(catalog, 0, LibraryFilter::Favorites, "");
  assert(grid.Count() == favorites_before);

  grid.Refresh(catalog, 0, LibraryFilter::Recent, "");
  const int recent_before = grid.Count();
  catalog.MarkOpened(0, 10);
  grid.Refresh(catalog, 0, LibraryFilter::Recent, "");
  assert(grid.Count() >= recent_before);
  assert(grid.LibraryIndexAt(0) == 10);

  grid.Refresh(catalog, 0, LibraryFilter::All, "not-a-demo-title");
  assert(grid.Count() == 0);
  return 0;
}
