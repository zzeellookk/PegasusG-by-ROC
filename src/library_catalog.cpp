#include "library_catalog.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>

namespace {

struct Rgb {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

std::string LowerAscii(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

bool ContainsQuery(const LibraryItem &item, const std::string &query) {
  if (query.empty()) return true;
  const std::string lower_query = LowerAscii(query);
  return LowerAscii(item.title).find(lower_query) != std::string::npos ||
         LowerAscii(item.metadata).find(lower_query) != std::string::npos ||
         LowerAscii(item.path).find(lower_query) != std::string::npos;
}

std::vector<LibraryItem> MakeDemoItems(const ModuleDefinition &module, int count,
                                       int color_seed) {
  static const char *kSuffixes[] = {
      "春日档案", "星海回声", "雨夜来信", "旧城漫游", "白昼梦境", "遥远灯塔",
      "玻璃花园", "夏末列车", "群青物语", "月下书简", "昨日旋律", "无声剧场",
  };
  static const Rgb kPastels[] = {
      {255, 228, 236}, {229, 242, 255}, {234, 245, 232}, {255, 242, 218},
      {238, 232, 255}, {226, 247, 244}, {255, 232, 218}, {236, 239, 245},
  };

  std::vector<LibraryItem> items;
  items.reserve(count);
  for (int i = 0; i < count; ++i) {
    LibraryItem item;
    item.id = module.id + ":demo:" + std::to_string(i);
    item.module_id = module.id;
    item.title = module.title + " · " + kSuffixes[i % 12];
    item.metadata = i % 3 == 0 ? "最近打开" : (i % 3 == 1 ? "已收藏" : "未开始");
    item.path = "/demo/" + module.id + "/" + std::to_string(i);
    item.favorite = i % 5 == 1 || i == 2;
    item.recent_order = i < 4 ? 100 - i : 0;
    item.progress_percent = i % 4 == 0 ? 15 + (i * 7) % 80 : -1;
    const Rgb base = kPastels[(color_seed + i) % 8];
    item.red = base.r;
    item.green = base.g;
    item.blue = base.b;
    items.push_back(std::move(item));
  }
  return items;
}

int MaxRecentOrder(const std::vector<std::vector<LibraryItem>> &modules) {
  int order = 0;
  for (const auto &items : modules) {
    for (const auto &item : items) {
      order = std::max(order, item.recent_order);
    }
  }
  return order;
}

}  // namespace

LibraryFilter LibraryFilterFromIndex(int index) {
  switch (index) {
    case 1: return LibraryFilter::Recent;
    case 2: return LibraryFilter::Favorites;
    default: return LibraryFilter::All;
  }
}

LibraryCatalog::LibraryCatalog(std::vector<std::vector<LibraryItem>> modules)
    : modules_(std::move(modules)), next_recent_order_(MaxRecentOrder(modules_) + 1) {}

LibraryCatalog LibraryCatalog::Demo(const std::vector<ModuleDefinition> &modules) {
  std::vector<std::vector<LibraryItem>> items;
  items.reserve(modules.size());
  for (int i = 0; i < static_cast<int>(modules.size()); ++i) {
    int count = 12;
    if (modules[i].id == "novel") count = 16;
    if (modules[i].id == "comic") count = 14;
    if (modules[i].id == "music") count = 18;
    if (modules[i].id == "settings") count = 9;
    if (modules[i].id == "power") count = 6;
    items.push_back(MakeDemoItems(modules[i], count, i));
  }
  return LibraryCatalog(std::move(items));
}

int LibraryCatalog::ModuleCount() const {
  return static_cast<int>(modules_.size());
}

bool LibraryCatalog::Empty() const {
  return TotalItemCount() == 0;
}

size_t LibraryCatalog::TotalItemCount() const {
  size_t total = 0;
  for (const auto &items : modules_) total += items.size();
  return total;
}

const std::vector<LibraryItem> &LibraryCatalog::ItemsForModule(int module_index) const {
  static const std::vector<LibraryItem> kEmpty;
  if (module_index < 0 || module_index >= ModuleCount()) return kEmpty;
  return modules_[module_index];
}

const LibraryItem *LibraryCatalog::Item(int module_index, int item_index) const {
  const auto &items = ItemsForModule(module_index);
  if (item_index < 0 || item_index >= static_cast<int>(items.size())) return nullptr;
  return &items[item_index];
}

std::vector<int> LibraryCatalog::Query(int module_index, LibraryFilter filter,
                                       const std::string &search_query) const {
  std::vector<int> indices;
  const auto &items = ItemsForModule(module_index);
  for (int i = 0; i < static_cast<int>(items.size()); ++i) {
    const LibraryItem &item = items[i];
    if (!ContainsQuery(item, search_query)) continue;
    if (filter == LibraryFilter::Recent && item.recent_order <= 0) continue;
    if (filter == LibraryFilter::Favorites && !item.favorite) continue;
    indices.push_back(i);
  }

  if (filter == LibraryFilter::Recent) {
    std::sort(indices.begin(), indices.end(), [&](int left, int right) {
      return items[left].recent_order > items[right].recent_order;
    });
  }
  return indices;
}

int LibraryCatalog::Count(int module_index, LibraryFilter filter,
                          const std::string &search_query) const {
  return static_cast<int>(Query(module_index, filter, search_query).size());
}

void LibraryCatalog::SetItemsForModule(int module_index,
                                       std::vector<LibraryItem> items) {
  if (module_index < 0) return;
  if (module_index >= ModuleCount()) modules_.resize(module_index + 1);
  modules_[module_index] = std::move(items);
  next_recent_order_ = MaxRecentOrder(modules_) + 1;
}

bool LibraryCatalog::ToggleFavorite(int module_index, int item_index) {
  const LibraryItem *item = Item(module_index, item_index);
  if (!item) return false;
  return SetFavorite(module_index, item_index, !item->favorite);
}

bool LibraryCatalog::SetFavorite(int module_index, int item_index, bool favorite) {
  if (module_index < 0 || module_index >= ModuleCount()) return false;
  auto &items = modules_[module_index];
  if (item_index < 0 || item_index >= static_cast<int>(items.size())) return false;
  items[item_index].favorite = favorite;
  items[item_index].metadata = items[item_index].favorite ? "已收藏" : "未开始";
  return items[item_index].favorite;
}

bool LibraryCatalog::MarkOpened(int module_index, int item_index) {
  if (module_index < 0 || module_index >= ModuleCount()) return false;
  auto &items = modules_[module_index];
  if (item_index < 0 || item_index >= static_cast<int>(items.size())) return false;
  items[item_index].recent_order = next_recent_order_++;
  if (items[item_index].progress_percent < 0) items[item_index].progress_percent = 1;
  items[item_index].metadata = "最近打开";
  return true;
}
