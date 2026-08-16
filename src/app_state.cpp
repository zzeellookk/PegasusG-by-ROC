#include "app.h"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <utility>

namespace {

int ModuleIndexById(const std::vector<ModuleDefinition> &modules, const std::string &id) {
  for (int i = 0; i < static_cast<int>(modules.size()); ++i) {
    if (modules[i].id == id) return i;
  }
  return -1;
}

}  // namespace

void RocShellApp::RefreshVisibleItems() {
  const LibraryCatalog &catalog = ActiveCatalog();
  grid_data_.Refresh(catalog, active_module_, LibraryFilterFromIndex(selected_filter_),
                     search_query_);
  if (grid_data_.Count() == 0) {
    selected_card_ = 0;
  } else {
    selected_card_ = std::clamp(selected_card_, 0, grid_data_.Count() - 1);
  }
}

int RocShellApp::CurrentItemCount() const {
  return grid_data_.Count();
}

int RocShellApp::SelectedLibraryItemIndex() const {
  return grid_data_.LibraryIndexAt(selected_card_);
}

void RocShellApp::ClampSelection() {
  if (grid_data_.Count() == 0) {
    selected_card_ = 0;
    scroll_row_ = 0;
    return;
  }
  selected_card_ = std::clamp(selected_card_, 0, grid_data_.Count() - 1);
}

void RocShellApp::SetStatus(std::string message, Uint32 duration_ms) {
  status_message_ = std::move(message);
  status_until_ = SDL_GetTicks() + duration_ms;
}

void RocShellApp::StartLibraryScan() {
  if (options_.scan_roots.empty() || scan_running_) return;
  scan_cancel_ = false;
  scan_ready_ = false;
  scan_running_ = true;
  scan_thread_ = std::thread([this]() {
    LibraryScanOptions scan_options;
    scan_options.roots = options_.scan_roots;
    scan_options.max_items = options_.max_scan_items;
    scan_result_ = ScanLibraryRoots(modules_, scan_options, &scan_cancel_);
    scan_ready_ = true;
  });
  SetStatus("正在扫描本地内容", 2200);
}

void RocShellApp::PollLibraryScan() {
  if (!scan_ready_) return;
  if (scan_thread_.joinable()) scan_thread_.join();
  scan_running_ = false;
  scan_ready_ = false;
  LibraryCatalog scanned(std::move(scan_result_));
  if (!scanned.Empty()) {
    catalog_ = std::move(scanned);
    RebuildSettingsCatalog();
    if (IsOnlineNovelMode()) RebuildOnlineCatalog();
    selected_card_ = 0;
    scroll_row_ = 0;
    SetStatus("已载入本地内容", 1800);
  } else {
    SetStatus("未扫描到本地内容，保留演示库", 2200);
  }
  RefreshVisibleItems();
  ClampSelection();
}

bool RocShellApp::IsNovelModule() const {
  return active_module_ >= 0 && active_module_ < static_cast<int>(modules_.size()) &&
         modules_[active_module_].id == "novel";
}

bool RocShellApp::IsSettingsModule() const {
  return active_module_ >= 0 && active_module_ < static_cast<int>(modules_.size()) &&
         modules_[active_module_].id == "settings";
}

bool RocShellApp::IsOnlineNovelMode() const {
  return IsNovelModule() && novel_source_mode_ == NovelSourceMode::Online;
}

bool RocShellApp::HasNovelSourceSwitch() const {
  return IsNovelModule();
}

const LibraryCatalog &RocShellApp::ActiveCatalog() const {
  if (IsOnlineNovelMode()) return online_catalog_;
  return catalog_;
}

LibraryCatalog &RocShellApp::ActiveCatalog() {
  if (IsOnlineNovelMode()) return online_catalog_;
  return catalog_;
}

void RocShellApp::SetNovelSourceMode(NovelSourceMode mode) {
  novel_source_mode_ = mode;
  selected_source_tab_ = mode == NovelSourceMode::Online ? 1 : 0;
  if (mode == NovelSourceMode::Online) {
    RebuildOnlineCatalog();
  }
  selected_card_ = 0;
  scroll_row_ = 0;
  RefreshVisibleItems();
  ClampSelection();
}

std::string RocShellApp::ActiveSourceName() const {
  if (IsOnlineNovelMode()) return online_sources_.SelectedSource().name;
  if (IsNovelModule()) return "本地小说库";
  return {};
}

void RocShellApp::LoadOnlineSources() {
  if (options_.online_sources_config.empty()) {
    options_.online_sources_config = FindFile({
        "online_sources.ini",
        "assets/config/online_sources.ini",
        "H700/launcher/online_sources.ini",
    });
  }
  if (!options_.online_sources_config.empty()) {
    online_sources_ = OnlineSourceConfig::LoadFromIni(options_.online_sources_config);
  } else {
    online_sources_ = OnlineSourceConfig::Defaults();
  }
  if (online_sources_.Empty()) online_sources_ = OnlineSourceConfig::Defaults();
  RebuildSettingsCatalog();
  RebuildOnlineCatalog();
  if (!options_.online_sources_config.empty() &&
      !std::filesystem::exists(options_.online_sources_config)) {
    online_sources_.SaveToIni(options_.online_sources_config);
  }
}

void RocShellApp::RebuildOnlineCatalog() {
  const int novel_index = ModuleIndexById(modules_, "novel");
  if (novel_index < 0) return;
  const auto &novel_module = modules_[novel_index];
  online_catalog_ = LibraryCatalog(std::vector<std::vector<LibraryItem>>(modules_.size()));
  online_catalog_.SetItemsForModule(
      novel_index, MakeOnlineNovelItems(novel_module, online_sources_.SelectedSource()));
}

void RocShellApp::RebuildSettingsCatalog() {
  const int settings_index = ModuleIndexById(modules_, "settings");
  if (settings_index < 0) return;
  catalog_.SetItemsForModule(
      settings_index, MakeOnlineSourceSettingsItems(modules_[settings_index], online_sources_));
}

void RocShellApp::SelectOnlineSource(int index) {
  online_sources_.SelectIndex(index);
  RebuildOnlineCatalog();
  RebuildSettingsCatalog();
  if (!options_.online_sources_config.empty()) {
    online_sources_.SaveToIni(options_.online_sources_config);
  }
  if (IsOnlineNovelMode()) {
    RefreshVisibleItems();
    ClampSelection();
  }
}

bool RocShellApp::HandleTextInput(const SDL_Event &event) {
  if (!search_active_) return false;
  if (event.type == SDL_TEXTINPUT) {
    search_query_ += event.text.text;
    RefreshVisibleItems();
    return true;
  }
  if (event.type == SDL_KEYDOWN) {
    const SDL_Keycode key = event.key.keysym.sym;
    if (key == SDLK_ESCAPE) {
      search_active_ = false;
      SDL_StopTextInput();
      return true;
    }
    if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
      search_active_ = false;
      SDL_StopTextInput();
      SetStatus(search_query_.empty() ? "搜索已取消" : "搜索已应用");
      RefreshVisibleItems();
      return true;
    }
    if (key == SDLK_BACKSPACE && !search_query_.empty()) {
      search_query_.pop_back();
      while (!search_query_.empty() &&
             (static_cast<unsigned char>(search_query_.back()) & 0xC0) == 0x80) {
        search_query_.pop_back();
      }
      RefreshVisibleItems();
      return true;
    }
  }
  return false;
}

bool RocShellApp::HandleAction(UiAction action) {
  const int item_count = CurrentItemCount();
  const int columns = std::max(1, layout_.grid_columns);

  if (online_book_menu_open_) {
    if (action == UiAction::Back || action == UiAction::Menu) {
      online_book_menu_open_ = false;
      focus_zone_ = FocusZone::Grid;
      SetStatus("已返回书库");
      return true;
    }
    if (action == UiAction::Up) {
      selected_online_action_ = (selected_online_action_ + 2) % 3;
      return true;
    }
    if (action == UiAction::Down) {
      selected_online_action_ = (selected_online_action_ + 1) % 3;
      return true;
    }
    if (action == UiAction::Confirm) {
      if (selected_online_action_ == 0) {
        SetStatus("下载到本地功能待接入");
      } else if (selected_online_action_ == 1) {
        SetStatus("在线阅读功能待接入");
      } else {
        online_book_menu_open_ = false;
        SetStatus("已返回书库");
      }
      return true;
    }
    return true;
  }

  if (action == UiAction::Search) {
    search_active_ = true;
    focus_zone_ = FocusZone::Grid;
    SDL_StartTextInput();
    SetStatus(IsOnlineNovelMode() ? "搜索在线书库" : "搜索本地内容");
    RefreshVisibleItems();
    return true;
  }

  if (action == UiAction::Menu) {
    if (search_active_) {
      search_active_ = false;
      SDL_StopTextInput();
    }
    if (focus_zone_ == FocusZone::Sidebar) {
      focus_zone_ = FocusZone::Grid;
    } else if (focus_zone_ == FocusZone::Sources) {
      focus_zone_ = FocusZone::Grid;
    } else {
      focus_zone_ = FocusZone::Sidebar;
      selected_sidebar_ = active_module_;
    }
    return true;
  }

  if (action == UiAction::Back) {
    if (search_active_) {
      search_active_ = false;
      SDL_StopTextInput();
      RefreshVisibleItems();
      return true;
    }
    if (!search_query_.empty()) {
      search_query_.clear();
      RefreshVisibleItems();
      SetStatus("搜索已清除");
      return true;
    }
    if (focus_zone_ == FocusZone::Sidebar) {
      focus_zone_ = FocusZone::Grid;
    } else if (focus_zone_ == FocusZone::Sources) {
      focus_zone_ = FocusZone::Grid;
    } else if (focus_zone_ == FocusZone::Filters) {
      focus_zone_ = FocusZone::Grid;
    } else {
      focus_zone_ = FocusZone::Sidebar;
      selected_sidebar_ = active_module_;
    }
    return true;
  }

  if (action == UiAction::TabPrevious || action == UiAction::TabNext) {
    const int delta = action == UiAction::TabPrevious ? -1 : 1;
    selected_filter_ = (selected_filter_ + delta + 3) % 3;
    selected_card_ = 0;
    scroll_row_ = 0;
    RefreshVisibleItems();
    return true;
  }

  if (focus_zone_ == FocusZone::Sidebar) {
    if (action == UiAction::Up) {
      selected_sidebar_ = (selected_sidebar_ - 1 + static_cast<int>(modules_.size())) %
                          static_cast<int>(modules_.size());
    } else if (action == UiAction::Down) {
      selected_sidebar_ = (selected_sidebar_ + 1) % static_cast<int>(modules_.size());
    } else if (action == UiAction::Confirm || action == UiAction::Right) {
      active_module_ = selected_sidebar_;
      selected_card_ = 0;
      scroll_row_ = 0;
      RefreshVisibleItems();
      if (IsNovelModule() && action == UiAction::Right) {
        focus_zone_ = FocusZone::Sources;
      } else if (action == UiAction::Right) {
        focus_zone_ = FocusZone::Filters;
      } else {
        focus_zone_ = FocusZone::Grid;
      }
    }
    return true;
  }

  if (focus_zone_ == FocusZone::Sources) {
    if (!HasNovelSourceSwitch()) {
      focus_zone_ = FocusZone::Grid;
      return true;
    }
    if (action == UiAction::Left) {
      SetNovelSourceMode(NovelSourceMode::Local);
      SetStatus("已切换到本地小说库");
    } else if (action == UiAction::Right) {
      SetNovelSourceMode(NovelSourceMode::Online);
      SetStatus("已切换到在线小说库");
    } else if (action == UiAction::Confirm || action == UiAction::Down) {
      focus_zone_ = FocusZone::Filters;
    }
    return true;
  }

  if (focus_zone_ == FocusZone::Filters) {
    if (action == UiAction::Left) {
      if (selected_filter_ > 0) {
        --selected_filter_;
        selected_card_ = 0;
        scroll_row_ = 0;
        RefreshVisibleItems();
      }
    } else if (action == UiAction::Right) {
      const int next_filter = std::min(2, selected_filter_ + 1);
      if (next_filter != selected_filter_) {
        selected_filter_ = next_filter;
        selected_card_ = 0;
        scroll_row_ = 0;
        RefreshVisibleItems();
      }
    } else if (action == UiAction::Up && HasNovelSourceSwitch()) {
      focus_zone_ = FocusZone::Sources;
    } else if (action == UiAction::Down || action == UiAction::Confirm) {
      focus_zone_ = FocusZone::Grid;
    }
    return true;
  }

  if (focus_zone_ == FocusZone::Grid) {
    const bool settings_mode = IsSettingsModule();
    const bool online_novel = IsOnlineNovelMode();
    if (settings_mode && (action == UiAction::Confirm || action == UiAction::ContextPrimary)) {
      const int source_index = SelectedLibraryItemIndex();
      if (source_index >= 0) {
        SelectOnlineSource(source_index);
        SetStatus("已切换设置中的在线书源");
      }
      return true;
    }
    if (action == UiAction::ContextPrimary) {
      const int item_index = SelectedLibraryItemIndex();
      if (item_index >= 0) {
        ActiveCatalog().SetFavorite(active_module_, item_index, true);
        SetStatus("已加入收藏");
        RefreshVisibleItems();
        ClampSelection();
      }
    } else if (action == UiAction::ContextSecondary) {
      const int item_index = SelectedLibraryItemIndex();
      if (item_index >= 0) {
        ActiveCatalog().SetFavorite(active_module_, item_index, false);
        SetStatus("已取消收藏");
        RefreshVisibleItems();
        ClampSelection();
      }
    } else if (action == UiAction::Confirm) {
      const int item_index = SelectedLibraryItemIndex();
      if (item_index >= 0) {
        if (online_novel) {
          online_book_menu_open_ = true;
          selected_online_action_ = 0;
          focus_zone_ = FocusZone::OnlineBookMenu;
          SetStatus("在线书籍操作菜单");
        } else {
          catalog_.MarkOpened(active_module_, item_index);
          SetStatus("已记录最近打开");
          RefreshVisibleItems();
          ClampSelection();
        }
      }
    } else if (action == UiAction::Left) {
      if (selected_card_ % columns != 0) --selected_card_;
    } else if (action == UiAction::Right) {
      if (selected_card_ + 1 < item_count && selected_card_ % columns != columns - 1) {
        ++selected_card_;
      }
    } else if (action == UiAction::Up) {
      if (selected_card_ < columns) {
        focus_zone_ = FocusZone::Filters;
      } else {
        selected_card_ -= columns;
      }
    } else if (action == UiAction::Down) {
      if (selected_card_ + columns < item_count) selected_card_ += columns;
    } else if (action == UiAction::PagePrevious) {
      selected_card_ = std::max(0, selected_card_ - columns * 2);
    } else if (action == UiAction::PageNext) {
      selected_card_ = item_count == 0 ? 0
                                       : std::min(item_count - 1, selected_card_ + columns * 2);
    }
    return true;
  }
  return false;
}

void RocShellApp::RecalculateLayout() {
  int width = options_.width;
  int height = options_.height;
  SDL_GetRendererOutputSize(renderer_, &width, &height);
  display_ = ResolveDisplayMetrics(width, height, options_.density);
  SDL_RenderSetLogicalSize(renderer_, display_.logical_width, display_.logical_height);
  layout_ = ResolveLayout(display_.logical_width, display_.logical_height,
                          options_.safe_area, display_.density);
  layout_.display = display_;
  const int columns = std::max(1, layout_.grid_columns);
  const int selected_row = selected_card_ / columns;
  const int row_height = layout_.card_cover_height + layout_.card_footer_height + layout_.grid_gap;
  const int visible_rows = std::max(1, (layout_.grid_height + layout_.grid_gap) / row_height);
  scroll_row_ = std::clamp(scroll_row_, std::max(0, selected_row - visible_rows + 1),
                           selected_row);
  if (options_.diagnostics) LogDiagnostics("layout");
}

void RocShellApp::LogDiagnostics(const char *reason) const {
  int window_width = 0;
  int window_height = 0;
  SDL_GetWindowSize(window_, &window_width, &window_height);

  SDL_DisplayMode mode{};
  const int display_index = SDL_GetWindowDisplayIndex(window_);
  const bool has_display_mode =
      display_index >= 0 && SDL_GetCurrentDisplayMode(display_index, &mode) == 0;

  std::cerr << "[roc-front] " << reason
            << " window=" << window_width << 'x' << window_height
            << " framebuffer=" << display_.framebuffer_width << 'x'
            << display_.framebuffer_height
            << " logical=" << display_.logical_width << 'x' << display_.logical_height
            << " scale=" << std::fixed << std::setprecision(3) << display_.window_scale
            << " density=" << std::setprecision(2) << display_.density
            << " mode=" << LayoutModeName(layout_.mode)
            << " columns=" << layout_.grid_columns
            << " card=" << layout_.card_width << 'x' << layout_.card_cover_height
            << " safe=" << layout_.safe_area.left << ',' << layout_.safe_area.top
            << ',' << layout_.safe_area.right << ',' << layout_.safe_area.bottom
            << " items=" << CurrentItemCount()
            << " filter=" << selected_filter_;
  if (has_display_mode) {
    std::cerr << " display=" << mode.w << 'x' << mode.h << '@' << mode.refresh_rate;
  }
  std::cerr << '\n';
}
