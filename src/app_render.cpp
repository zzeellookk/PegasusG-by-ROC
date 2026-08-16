#include "app.h"

#include "ui_draw.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

std::string CacheKey(const std::string &text, int size, SDL_Color color) {
  return text + "#" + std::to_string(size) + "#" + std::to_string(color.r) + "," +
         std::to_string(color.g) + "," + std::to_string(color.b) + "," +
         std::to_string(color.a);
}

}  // namespace

void RocShellApp::Render() {
  using namespace roc_ui;
  SetColor(renderer_, kBackground);
  SDL_RenderClear(renderer_);
  RenderTopBar();
  RenderFilterBar();
  RenderLibrary();
  if (focus_zone_ == FocusZone::Sidebar) RenderSidebar();
  if (online_book_menu_open_) RenderOnlineBookMenu();
  RenderToast();
  RenderBottomBar();
}

void RocShellApp::RenderTopBar() {
  using namespace roc_ui;
  const SDL_Rect bar{0, 0, layout_.viewport_width, layout_.top_bar_height};
  Fill(renderer_, bar, kTopBar);

  const int small = layout_.top_bar_height <= 54 ? 15 : 17;
  const int icon_size = layout_.top_bar_height <= 54 ? 20 : 24;
  const int avatar_size = layout_.top_bar_height <= 54 ? 38 : 44;
  SDL_Rect avatar{layout_.safe_area.left + layout_.content_padding,
                  (layout_.top_bar_height - avatar_size) / 2, avatar_size,
                  avatar_size};
  RenderAvatar(avatar);

  const int right_edge = layout_.viewport_width - layout_.safe_area.right - 18;
  SDL_Rect battery{right_edge - 86, layout_.top_bar_height / 2 - 6, 24, 12};
  const bool show_secondary_status = layout_.viewport_width >= 520;
  const int right_controls_left = show_secondary_status ? battery.x - 72 : battery.x - 12;
  const int search_gap = layout_.viewport_width < 600 ? 10 : 14;
  const int search_pill_height = layout_.top_bar_height <= 54 ? 34 : 38;
  const int search_pill_x = avatar.x + avatar.w + search_gap;
  const int desired_search_width = layout_.viewport_width < 600 ? 176 : 430;
  const int available_search_width =
      std::max(88, right_controls_left - search_pill_x - 12);
  const int search_pill_width =
      std::max(88, std::min(desired_search_width, available_search_width));
  SDL_Rect search_pill{search_pill_x,
                       (layout_.top_bar_height - search_pill_height) / 2,
                       search_pill_width, search_pill_height};
  FillRoundedRect(renderer_, search_pill, search_pill.h / 2,
                  search_active_ || !search_query_.empty()
                      ? kSurfaceSelected
                      : SDL_Color{241, 242, 243, 255});
  if (search_active_) Stroke(renderer_, Inset(search_pill, 1), kFocus);

  SDL_Rect search_icon{search_pill.x + 12,
                       (layout_.top_bar_height - icon_size) / 2, icon_size, icon_size};
  SetColor(renderer_, kMuted);
  const int lens_radius = std::max(4, icon_size / 3);
  StrokeCircle(renderer_, search_icon.x + lens_radius, search_icon.y + lens_radius,
               lens_radius);
  SDL_RenderDrawLine(renderer_, search_icon.x + lens_radius * 2 - 1,
                     search_icon.y + lens_radius * 2 - 1, search_icon.x + search_icon.w,
                     search_icon.y + search_icon.h);

  const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm local_time{};
#ifdef _WIN32
  localtime_s(&local_time, &now);
#else
  localtime_r(&now, &local_time);
#endif
  std::ostringstream time_text;
  time_text << std::put_time(&local_time, "%H:%M");
  DrawText(time_text.str(), right_edge,
           (layout_.top_bar_height - small) / 2 - 2, small, kText, 0, true);

  Stroke(renderer_, battery, kText);
  SDL_Rect battery_tip{battery.x + battery.w, battery.y + 4, 3, 6};
  Fill(renderer_, battery_tip, kText);
  SDL_Rect battery_level{battery.x + 3, battery.y + 3, 15, battery.h - 6};
  Fill(renderer_, battery_level, kFocus);

  if (show_secondary_status) {
    SetColor(renderer_, kMuted);
    StrokeCircle(renderer_, battery.x - 24, layout_.top_bar_height / 2, 8);
    SDL_RenderDrawLine(renderer_, battery.x - 32, layout_.top_bar_height / 2,
                       battery.x - 16, layout_.top_bar_height / 2);
    SDL_Rect settings_icon{battery.x - 62, layout_.top_bar_height / 2 - 9, 18, 18};
    RenderIcon(ModuleIcon::Settings, settings_icon, kMuted);
  }

  const int search_text_x = search_icon.x + icon_size + 12;
  const int search_text_width =
      std::max(0, search_pill.x + search_pill.w - 12 - search_text_x);
  const std::string search_text =
      search_active_
          ? (search_query_.empty() ? std::string("输入搜索词") : search_query_)
          : (!search_query_.empty()
                 ? search_query_
                 : (layout_.viewport_width < 600
                        ? std::string("搜索")
                        : (IsOnlineNovelMode() ? std::string("搜索在线书库……")
                                               : std::string("搜索本地内容……"))));
  DrawText(search_text, search_text_x, (layout_.top_bar_height - small) / 2 - 2,
           small, kMuted, search_text_width);
}

void RocShellApp::RenderAvatar(const SDL_Rect &bounds) {
  using namespace roc_ui;
  const int radius = std::min(bounds.w, bounds.h) / 2;
  const int center_x = bounds.x + bounds.w / 2;
  const int center_y = bounds.y + bounds.h / 2;

  FillCircle(renderer_, center_x + 1, center_y + 2, radius,
             SDL_Color{207, 213, 222, 72});
  FillCircle(renderer_, center_x, center_y, radius, SDL_Color{255, 255, 255, 255});

  SDL_Rect image_bounds = Inset(bounds, 3);
  if (avatar_texture_) {
    SDL_RenderCopy(renderer_, avatar_texture_, nullptr, &image_bounds);
  } else {
    FillCircle(renderer_, center_x, center_y, radius - 3, kSurfaceSelected);
    FillCircle(renderer_, center_x, center_y - radius / 4, std::max(4, radius / 4),
               SDL_Color{251, 114, 153, 220});
    SDL_Rect shoulders{center_x - radius / 2, center_y + radius / 5, radius,
                       std::max(6, radius / 3)};
    FillRoundedRect(renderer_, shoulders, shoulders.h / 2,
                    SDL_Color{251, 114, 153, 150});
  }

  SetColor(renderer_, kAccent);
  StrokeCircle(renderer_, center_x, center_y, radius);
  SetColor(renderer_, SDL_Color{255, 255, 255, 190});
  StrokeCircle(renderer_, center_x, center_y, radius - 1);
}

void RocShellApp::RenderSidebar() {
  using namespace roc_ui;
  const int top = layout_.safe_area.top + layout_.top_bar_height;
  const int bottom =
      layout_.viewport_height - layout_.safe_area.bottom - layout_.bottom_bar_height;
  SDL_Rect veil{layout_.safe_area.left, top,
                layout_.viewport_width - layout_.safe_area.left - layout_.safe_area.right,
                bottom - top};
  Fill(renderer_, veil, SDL_Color{236, 240, 245, 212});

  SDL_Rect sidebar{layout_.safe_area.left, top, layout_.sidebar_width, bottom - top};
  Fill(renderer_, sidebar, kSidebar);
  SDL_Rect shadow{sidebar.x + sidebar.w, sidebar.y, 10, sidebar.h};
  Fill(renderer_, shadow, SDL_Color{208, 214, 222, 90});

  const int icon_size = 24;
  const int start_y =
      top + std::max(4, (sidebar.h - layout_.sidebar_item_height *
                                         static_cast<int>(modules_.size())) /
                            2);

  for (int index = 0; index < static_cast<int>(modules_.size()); ++index) {
    SDL_Rect item{sidebar.x, start_y + index * layout_.sidebar_item_height,
                  sidebar.w, layout_.sidebar_item_height};
    const bool active = index == active_module_;
    const bool focused = focus_zone_ == FocusZone::Sidebar && index == selected_sidebar_;
    if (active) Fill(renderer_, item, kSurface);
    if (focused) {
      Fill(renderer_, item, kSurfaceSelected);
      SDL_Rect accent{item.x, item.y + 8, 4, item.h - 16};
      FillRoundedRect(renderer_, accent, 2, kAccent);
    }

    SDL_Rect icon_bounds{item.x + 22, item.y + (item.h - icon_size) / 2,
                         icon_size, icon_size};
    RenderIcon(modules_[index].icon, icon_bounds, focused || active ? kAccent : kMuted);
    const int font_size = layout_.sidebar_item_height >= 56 ? 18 : 16;
    DrawText(modules_[index].title, icon_bounds.x + icon_bounds.w + 16,
             item.y + (item.h - font_size) / 2 - 2, font_size,
             focused || active ? kText : kMuted, item.w - 72);
  }
}

void RocShellApp::RenderFilterBar() {
  using namespace roc_ui;
  const int y = layout_.safe_area.top + layout_.top_bar_height;
  SDL_Rect bar{layout_.content_x, y, layout_.content_width, layout_.filter_bar_height};
  Fill(renderer_, bar, kTopBar);

  const LibraryCatalog &catalog = ActiveCatalog();
  const int item_count = catalog.Count(active_module_, LibraryFilter::All, search_query_);
  const std::string filters[] = {
      "全部 " + std::to_string(item_count),
      "最近 " + std::to_string(catalog.Count(active_module_, LibraryFilter::Recent,
                                             search_query_)),
      "收藏 " + std::to_string(catalog.Count(active_module_, LibraryFilter::Favorites,
                                             search_query_)),
  };
  const int font_size = layout_.mode == LayoutMode::Compact ? 14 : 16;
  const int pill_height = layout_.mode == LayoutMode::Compact ? 32 : 38;
  const int pill_width =
      layout_.mode == LayoutMode::Compact ? 96 : (layout_.viewport_height >= 900 ? 144 : 128);
  const int pill_gap = layout_.mode == LayoutMode::Compact ? 8 : 14;
  const int filter_width = pill_width * 3 + pill_gap * 2;
  int x = layout_.content_x + (layout_.content_width - filter_width) / 2;

  for (int index = 0; index < 3; ++index) {
    SDL_Rect pill{x, y + (bar.h - pill_height) / 2, pill_width, pill_height};
    const bool selected = index == selected_filter_;
    const bool focused = focus_zone_ == FocusZone::Filters && selected;
    if (selected) {
      FillRoundedRect(renderer_, pill, pill.h / 2, focused ? kSurfaceSelected : kSurface);
    }
    if (focused) Stroke(renderer_, Inset(pill, 1), kFocus);
    int text_width = 0;
    if (TTF_Font *font = Font(font_size)) {
      TTF_SizeUTF8(font, filters[index].c_str(), &text_width, nullptr);
    }
    DrawText(filters[index], pill.x + std::max(5, (pill.w - text_width) / 2),
             pill.y + (pill.h - font_size) / 2 - 2, font_size,
             selected ? kAccent : kMuted, pill.w - 10);
    x += pill_width + pill_gap;
  }

  if (layout_.viewport_width >= 560) {
    const int shoulder_width = layout_.mode == LayoutMode::Compact ? 40 : 46;
    const int shoulder_height = layout_.mode == LayoutMode::Compact ? 26 : 30;
    SDL_Rect left{layout_.safe_area.left + layout_.content_padding,
                  y + (bar.h - shoulder_height) / 2, shoulder_width, shoulder_height};
    SDL_Rect right{layout_.viewport_width - layout_.safe_area.right -
                       layout_.content_padding - shoulder_width,
                   left.y, shoulder_width, shoulder_height};
    FillRoundedRect(renderer_, left, 4, kAccent);
    FillRoundedRect(renderer_, right, 4, kAccent);
    DrawText("L1", left.x + 10, left.y + (left.h - 13) / 2 - 2, 13, kInk);
    DrawText("R1", right.x + 10, right.y + (right.h - 13) / 2 - 2, 13, kInk);
  }
}

void RocShellApp::RenderNovelSourceSwitch(int header_y) {
  using namespace roc_ui;
  const int font_size = layout_.mode == LayoutMode::Compact ? 14 : 16;
  const int switch_height = std::max(24, layout_.section_header_height - 8);
  const int gap = layout_.mode == LayoutMode::Compact ? 8 : 12;
  const int max_tab_width = layout_.mode == LayoutMode::Compact ? 118 : 150;
  const int tab_width =
      std::max(90, std::min(max_tab_width, (layout_.grid_width - gap - 54) / 2));
  const int total_width = tab_width * 2 + gap;
  const int start_x = layout_.grid_x + std::max(0, (layout_.grid_width - total_width) / 2);
  const int top = header_y + std::max(2, (layout_.section_header_height - switch_height) / 2);
  const char *labels[] = {"本地小说库", "在线小说库"};

  for (int index = 0; index < 2; ++index) {
    const bool selected =
        (index == 0 && novel_source_mode_ == NovelSourceMode::Local) ||
        (index == 1 && novel_source_mode_ == NovelSourceMode::Online);
    const bool focused = focus_zone_ == FocusZone::Sources && index == selected_source_tab_;
    SDL_Rect tab{start_x + index * (tab_width + gap), top, tab_width, switch_height};
    if (selected || focused) {
      FillRoundedRect(renderer_, tab, tab.h / 2,
                      selected ? kSurfaceSelected : SDL_Color{255, 255, 255, 255});
    }
    if (selected) {
      SDL_Rect slider{tab.x + 12, tab.y + tab.h - 5, tab.w - 24, 4};
      FillRoundedRect(renderer_, slider, 2, kAccent);
    }
    if (focused) Stroke(renderer_, Inset(tab, 1), kFocus);

    int text_width = 0;
    if (TTF_Font *font = Font(font_size)) {
      TTF_SizeUTF8(font, labels[index], &text_width, nullptr);
    }
    DrawText(labels[index], tab.x + std::max(4, (tab.w - text_width) / 2),
             tab.y + (tab.h - font_size) / 2 - 2, font_size,
             selected ? kAccent : kMuted, tab.w - 8);
  }

  if (IsOnlineNovelMode() && layout_.viewport_width >= 640) {
    DrawText(ActiveSourceName(),
             layout_.content_x + layout_.content_width - layout_.content_padding,
             header_y + std::max(2, (layout_.section_header_height - 12) / 2 - 2),
             12, kDim, layout_.grid_width / 4, true);
  }
}

void RocShellApp::RenderLibrary() {
  using namespace roc_ui;
  const int header_y =
      layout_.safe_area.top + layout_.top_bar_height + layout_.filter_bar_height;
  if (HasNovelSourceSwitch()) {
    RenderNovelSourceSwitch(header_y);
  } else {
    DrawText(modules_[active_module_].section_title,
             layout_.content_x + layout_.content_padding,
             header_y + std::max(2, (layout_.section_header_height - 16) / 2 - 2),
             layout_.mode == LayoutMode::Compact ? 14 : 16, kMuted,
             layout_.grid_width - 100);
  }
  DrawText(std::to_string(CurrentItemCount()),
           layout_.content_x + layout_.content_width - layout_.content_padding,
           header_y + std::max(2, (layout_.section_header_height - 14) / 2 - 2), 13,
           kDim, 0, true);
  SDL_Rect divider{layout_.grid_x, header_y + layout_.section_header_height - 1,
                   layout_.grid_width, 1};
  Fill(renderer_, divider, kDivider);

  SDL_Rect clip{layout_.grid_x - 5, layout_.grid_y, layout_.grid_width + 10,
                layout_.grid_height};
  SDL_RenderSetClipRect(renderer_, &clip);

  const int columns = std::max(1, layout_.grid_columns);
  const int row_height = layout_.card_cover_height + layout_.card_footer_height +
                         layout_.grid_gap;
  const int selected_row = selected_card_ / columns;
  const int visible_rows = std::max(1, (layout_.grid_height + layout_.grid_gap) / row_height);
  if (selected_row < scroll_row_) scroll_row_ = selected_row;
  if (selected_row >= scroll_row_ + visible_rows) {
    scroll_row_ = selected_row - visible_rows + 1;
  }
  scroll_row_ = std::max(0, scroll_row_);

  if (grid_data_.Count() == 0) {
    DrawText(search_active_ ? "没有匹配内容" : "没有可显示的内容", layout_.grid_x,
             layout_.grid_y + 20, layout_.mode == LayoutMode::Compact ? 14 : 16,
             kMuted, layout_.grid_width);
    SDL_RenderSetClipRect(renderer_, nullptr);
    return;
  }

  for (int index = 0; index < grid_data_.Count(); ++index) {
    const LibraryItem *resolved_item = grid_data_.ItemAt(ActiveCatalog(), index);
    if (!resolved_item) continue;
    const LibraryItem &item = *resolved_item;
    const int row = index / columns;
    const int column = index % columns;
    const int y = layout_.grid_y + (row - scroll_row_) * row_height;
    if (y + layout_.card_cover_height + layout_.card_footer_height < layout_.grid_y ||
        y >= layout_.grid_y + layout_.grid_height) {
      continue;
    }

    SDL_Rect cover{layout_.grid_x + column * (layout_.card_width + layout_.grid_gap),
                   y, layout_.card_width, layout_.card_cover_height};
    SDL_Rect footer{cover.x, cover.y + cover.h, cover.w, layout_.card_footer_height};
    const SDL_Color tint = ColorForItem(item);
    SDL_Rect card_shadow{cover.x + 2, cover.y + 3, cover.w, cover.h + footer.h};
    FillRoundedRect(renderer_, card_shadow, 8, SDL_Color{214, 220, 228, 110});
    FillRoundedRect(renderer_, cover, 8, tint);
    SDL_Rect cover_inner = Inset(cover, std::max(4, layout_.card_width / 22));
    Stroke(renderer_, cover_inner, SDL_Color{255, 255, 255, 130});

    if (cover_placeholder_) {
      SDL_SetTextureColorMod(cover_placeholder_, tint.r, tint.g, tint.b);
      SDL_SetTextureAlphaMod(cover_placeholder_, 84);
      SDL_RenderCopy(renderer_, cover_placeholder_, nullptr, &cover);
      SDL_SetTextureColorMod(cover_placeholder_, 255, 255, 255);
      SDL_SetTextureAlphaMod(cover_placeholder_, 255);
    }

    if (item.favorite) {
      const int badge = layout_.mode == LayoutMode::Compact ? 22 : 26;
      SDL_Rect favorite_badge{cover.x + cover.w - badge - 8, cover.y + 8, badge,
                              badge};
      FillCircle(renderer_, favorite_badge.x + badge / 2,
                 favorite_badge.y + badge / 2, badge / 2, kAccent);
      DrawText("*", favorite_badge.x + badge / 2 - 4,
               favorite_badge.y + badge / 2 - 9, 16, kInk);
    }

    if (item.progress_percent >= 0) {
      const int progress = std::clamp(item.progress_percent, 0, 100);
      SDL_Rect track{cover.x + 8, cover.y + cover.h - 9, cover.w - 16, 4};
      FillRoundedRect(renderer_, track, 2, SDL_Color{255, 255, 255, 170});
      SDL_Rect fill{track.x, track.y, (track.w * progress) / 100, track.h};
      FillRoundedRect(renderer_, fill, 2, kFocus);
    }

    if (!item.source_url.empty()) {
      SDL_Rect online_badge{cover.x + 8, cover.y + 8,
                            layout_.mode == LayoutMode::Compact ? 36 : 42,
                            layout_.mode == LayoutMode::Compact ? 20 : 22};
      FillRoundedRect(renderer_, online_badge, online_badge.h / 2, kAccent);
      DrawText("在线", online_badge.x + 6,
               online_badge.y + (online_badge.h - 12) / 2 - 2, 12, kInk,
               online_badge.w - 10);
    }

    const int mark_size = std::max(26, layout_.card_width / 4);
    SDL_Rect mark{cover.x + (cover.w - mark_size) / 2,
                  cover.y + (cover.h - mark_size) / 2, mark_size, mark_size};
    FillRoundedRect(renderer_, mark, 6, SDL_Color{255, 255, 255, 190});
    Stroke(renderer_, mark, SDL_Color{251, 114, 153, 150});
    DrawText(modules_[active_module_].title.substr(
                 0, std::min<size_t>(modules_[active_module_].title.size(), 3)),
             mark.x + 4, mark.y + mark.h / 2 - 8,
             layout_.mode == LayoutMode::Compact ? 13 : 15, kAccent, mark.w - 8);

    Fill(renderer_, footer, SDL_Color{255, 255, 255, 255});
    SDL_Rect footer_line{footer.x, footer.y, footer.w, 1};
    Fill(renderer_, footer_line, SDL_Color{239, 241, 244, 255});
    const int title_size = layout_.mode == LayoutMode::Compact ? 12 : 14;
    DrawText(item.title, footer.x + 7, footer.y + (footer.h - title_size) / 2 - 2,
             title_size, kText, footer.w - 14);

    const bool selected = focus_zone_ == FocusZone::Grid && index == selected_card_;
    if (selected) {
      SDL_Rect frame{cover.x, cover.y, cover.w, cover.h + footer.h};
      SetColor(renderer_, kFocus);
      for (int thickness = 0; thickness < 3; ++thickness) {
        SDL_Rect layer = Inset(frame, thickness);
        SDL_RenderDrawRect(renderer_, &layer);
      }
      SDL_Rect focus_tab{frame.x, frame.y, frame.w, 4};
      Fill(renderer_, focus_tab, kAccent);
    }
  }
  SDL_RenderSetClipRect(renderer_, nullptr);
}

void RocShellApp::RenderOnlineBookMenu() {
  using namespace roc_ui;
  const LibraryItem *item = grid_data_.ItemAt(ActiveCatalog(), selected_card_);
  const std::string title = item ? item->title : "在线书籍";
  const std::string source = item ? item->metadata : ActiveSourceName();
  const std::string url = item ? item->source_url : online_sources_.SelectedSource().url;

  const int top = layout_.safe_area.top + layout_.top_bar_height;
  const int bottom =
      layout_.viewport_height - layout_.safe_area.bottom - layout_.bottom_bar_height;
  SDL_Rect veil{0, top, layout_.viewport_width, bottom - top};
  Fill(renderer_, veil, SDL_Color{246, 247, 249, 248});

  SDL_Rect accent{layout_.safe_area.left, top, 5, veil.h};
  Fill(renderer_, accent, kAccent);

  const int x = layout_.content_x + layout_.content_padding;
  const int width = layout_.content_width - layout_.content_padding * 2;
  const int title_size = layout_.mode == LayoutMode::Compact ? 20 : 24;
  const int body_size = layout_.mode == LayoutMode::Compact ? 13 : 15;
  int cursor_y = top + (layout_.mode == LayoutMode::Compact ? 18 : 26);
  DrawText("在线书籍", x, cursor_y, body_size, kMuted, width);
  cursor_y += layout_.mode == LayoutMode::Compact ? 22 : 30;
  DrawText(title, x, cursor_y, title_size, kText, width);
  cursor_y += layout_.mode == LayoutMode::Compact ? 30 : 38;
  DrawText(source + "  " + url, x, cursor_y, body_size, kDim, width);
  cursor_y += layout_.mode == LayoutMode::Compact ? 26 : 34;

  const char *actions[] = {"下载到本地", "在线阅读（预留）", "返回书库"};
  const char *descriptions[] = {
      "保存到本地小说库后用本地阅读器打开",
      "后续接入流式解析时启用",
      "不执行操作，回到当前在线书库",
  };
  const int row_height = layout_.mode == LayoutMode::Compact ? 58 : 68;
  for (int index = 0; index < 3; ++index) {
    SDL_Rect row{x, cursor_y + index * row_height, width, row_height - 8};
    const bool focused = index == selected_online_action_;
    FillRoundedRect(renderer_, row, 6,
                    focused ? kSurfaceSelected : SDL_Color{255, 255, 255, 255});
    if (focused) {
      SDL_Rect indicator{row.x, row.y + 8, 4, row.h - 16};
      FillRoundedRect(renderer_, indicator, 2, kAccent);
      Stroke(renderer_, Inset(row, 1), kFocus);
    }
    DrawText(actions[index], row.x + 18, row.y + 9,
             layout_.mode == LayoutMode::Compact ? 15 : 17,
             focused ? kAccent : kText, row.w - 36);
    DrawText(descriptions[index], row.x + 18, row.y + 31,
             body_size, kMuted, row.w - 36);
  }
}

void RocShellApp::RenderBottomBar() {
  using namespace roc_ui;
  const int y = layout_.viewport_height - layout_.safe_area.bottom - layout_.bottom_bar_height;
  SDL_Rect bar{0, y, layout_.viewport_width, layout_.bottom_bar_height};
  Fill(renderer_, bar, kTopBar);
  SDL_Rect line{0, y, layout_.viewport_width, 1};
  Fill(renderer_, line, kDivider);

  const int font_size = layout_.bottom_bar_height <= 40 ? 12 : 14;
  const int brand_height = layout_.bottom_bar_height <= 40 ? 22 : 26;
  SDL_Rect brand{layout_.safe_area.left + layout_.content_padding,
                 y + (layout_.bottom_bar_height - brand_height) / 2,
                 layout_.bottom_bar_height <= 40 ? 42 : 50, brand_height};
  FillRoundedRect(renderer_, brand, brand.h / 2, kAccent);
  DrawText("ROC", brand.x + 7, brand.y + (brand.h - font_size) / 2 - 2, font_size,
           kInk, brand.w - 10);
  DrawText("菜单", brand.x + brand.w + 8,
           y + (layout_.bottom_bar_height - font_size) / 2 - 2, font_size, kText, 50);

  const bool compact_bar = layout_.bottom_bar_height <= 40;
  const int right_edge = layout_.viewport_width - layout_.safe_area.right -
                         layout_.content_padding;
  if (online_book_menu_open_) {
    const int a_x = right_edge - (compact_bar ? 144 : 178);
    const int b_x = a_x + (compact_bar ? 76 : 96);
    RenderButtonHint(a_x, y, "A", "执行");
    RenderButtonHint(b_x, y, "B", "返回");
  } else if (focus_zone_ == FocusZone::Grid && IsSettingsModule()) {
    const int a_x = right_edge - (compact_bar ? 144 : 178);
    const int b_x = a_x + (compact_bar ? 76 : 96);
    RenderButtonHint(a_x, y, "A", "选书源");
    RenderButtonHint(b_x, y, "B", "返回");
  } else if (focus_zone_ == FocusZone::Grid) {
    const int b_x = right_edge - (compact_bar ? 68 : 82);
    const int a_x = b_x - (compact_bar ? 72 : 88);
    const int y_x = a_x - (compact_bar ? 112 : 132);
    const int x_x = y_x - (compact_bar ? 68 : 86);
    RenderButtonHint(x_x, y, "X", "收藏");
    RenderButtonHint(y_x, y, "Y", "取消收藏");
    RenderButtonHint(a_x, y, "A", "选择");
    RenderButtonHint(b_x, y, "B", !search_query_.empty() ? "清搜索" : "返回");
  } else {
    const int a_x = right_edge - (compact_bar ? 144 : 178);
    const int b_x = a_x + (compact_bar ? 76 : 96);
    RenderButtonHint(a_x, y, "A", focus_zone_ == FocusZone::Sidebar ? "进入" : "选择");
    RenderButtonHint(b_x, y, "B",
                     !search_query_.empty()
                         ? "清搜索"
                         : (focus_zone_ == FocusZone::Sidebar ? "关闭" : "返回"));
  }
}

void RocShellApp::RenderToast() {
  using namespace roc_ui;
  if (status_message_.empty() || SDL_GetTicks() > status_until_) return;
  const int font_size = layout_.mode == LayoutMode::Compact ? 13 : 15;
  int text_width = 0;
  if (TTF_Font *font = Font(font_size)) {
    TTF_SizeUTF8(font, status_message_.c_str(), &text_width, nullptr);
  }
  const int padding_x = 18;
  const int height = layout_.mode == LayoutMode::Compact ? 34 : 40;
  const int width = std::min(layout_.grid_width, std::max(180, text_width + padding_x * 2));
  SDL_Rect toast{layout_.content_x + (layout_.content_width - width) / 2,
                 layout_.viewport_height - layout_.safe_area.bottom -
                     layout_.bottom_bar_height - height - 12,
                 width, height};
  FillRoundedRect(renderer_, toast, height / 2, SDL_Color{255, 255, 255, 235});
  Stroke(renderer_, Inset(toast, 1), SDL_Color{224, 227, 232, 255});
  DrawText(status_message_, toast.x + padding_x,
           toast.y + (toast.h - font_size) / 2 - 2, font_size, kText,
           toast.w - padding_x * 2);
}

void RocShellApp::RenderIcon(ModuleIcon icon, const SDL_Rect &bounds, SDL_Color color) {
  using namespace roc_ui;
  SetColor(renderer_, color);
  const int left = bounds.x;
  const int top = bounds.y;
  const int right = bounds.x + bounds.w - 1;
  const int bottom = bounds.y + bounds.h - 1;
  const int mid_x = bounds.x + bounds.w / 2;
  const int mid_y = bounds.y + bounds.h / 2;

  switch (icon) {
    case ModuleIcon::Home:
      SDL_RenderDrawLine(renderer_, left + 2, mid_y, mid_x, top + 2);
      SDL_RenderDrawLine(renderer_, mid_x, top + 2, right - 2, mid_y);
      StrokeRect(renderer_, left + 5, mid_y, bounds.w - 10, bottom - mid_y - 1);
      break;
    case ModuleIcon::Novel:
      StrokeRect(renderer_, left + 3, top + 3, bounds.w - 6, bounds.h - 6);
      SDL_RenderDrawLine(renderer_, mid_x, top + 4, mid_x, bottom - 3);
      break;
    case ModuleIcon::Comic:
      StrokeRect(renderer_, left + 2, top + 2, bounds.w - 4, bounds.h - 4);
      SDL_RenderDrawLine(renderer_, mid_x, top + 3, mid_x, bottom - 2);
      SDL_RenderDrawLine(renderer_, left + 3, mid_y, right - 2, mid_y);
      break;
    case ModuleIcon::Video:
      StrokeRect(renderer_, left + 2, top + 4, bounds.w - 4, bounds.h - 8);
      SDL_RenderDrawLine(renderer_, mid_x - 3, mid_y - 5, mid_x + 5, mid_y);
      SDL_RenderDrawLine(renderer_, mid_x + 5, mid_y, mid_x - 3, mid_y + 5);
      SDL_RenderDrawLine(renderer_, mid_x - 3, mid_y + 5, mid_x - 3, mid_y - 5);
      break;
    case ModuleIcon::Music:
      SDL_RenderDrawLine(renderer_, mid_x, top + 2, mid_x, bottom - 5);
      SDL_RenderDrawLine(renderer_, mid_x, top + 2, right - 2, top + 6);
      SDL_RenderDrawLine(renderer_, right - 2, top + 6, right - 2, bottom - 8);
      StrokeRect(renderer_, mid_x - 5, bottom - 7, 6, 5);
      StrokeRect(renderer_, right - 6, bottom - 10, 6, 5);
      break;
    case ModuleIcon::Ons:
    case ModuleIcon::Krkr:
      StrokeRect(renderer_, left + 2, mid_y - 5, bounds.w - 4, 11);
      SDL_RenderDrawLine(renderer_, left + 6, mid_y, left + 12, mid_y);
      SDL_RenderDrawLine(renderer_, left + 9, mid_y - 3, left + 9, mid_y + 3);
      StrokeRect(renderer_, right - 10, mid_y - 2, 2, 2);
      StrokeRect(renderer_, right - 6, mid_y + 1, 2, 2);
      if (icon == ModuleIcon::Krkr) SDL_RenderDrawLine(renderer_, mid_x, top + 1, mid_x, top + 5);
      break;
    case ModuleIcon::Settings:
      StrokeRect(renderer_, mid_x - 5, mid_y - 5, 10, 10);
      StrokeRect(renderer_, mid_x - 2, mid_y - 2, 4, 4);
      SDL_RenderDrawLine(renderer_, mid_x, top, mid_x, top + 4);
      SDL_RenderDrawLine(renderer_, mid_x, bottom - 4, mid_x, bottom);
      SDL_RenderDrawLine(renderer_, left, mid_y, left + 4, mid_y);
      SDL_RenderDrawLine(renderer_, right - 4, mid_y, right, mid_y);
      break;
    case ModuleIcon::Power:
      SDL_RenderDrawLine(renderer_, mid_x, top + 1, mid_x, mid_y + 2);
      SDL_RenderDrawLine(renderer_, left + 5, top + 6, left + 2, mid_y);
      SDL_RenderDrawLine(renderer_, left + 2, mid_y, left + 5, bottom - 4);
      SDL_RenderDrawLine(renderer_, left + 5, bottom - 4, mid_x, bottom - 1);
      SDL_RenderDrawLine(renderer_, mid_x, bottom - 1, right - 5, bottom - 4);
      SDL_RenderDrawLine(renderer_, right - 5, bottom - 4, right - 2, mid_y);
      SDL_RenderDrawLine(renderer_, right - 2, mid_y, right - 5, top + 6);
      break;
  }
}

void RocShellApp::RenderButtonHint(int x, int y, const char *button, const char *label) {
  using namespace roc_ui;
  const int diameter = layout_.bottom_bar_height <= 40 ? 22 : 28;
  SDL_Rect outline{x, y + (layout_.bottom_bar_height - diameter) / 2, diameter, diameter};
  FillCircle(renderer_, outline.x + diameter / 2, outline.y + diameter / 2,
             diameter / 2, kAccent);
  DrawText(button, outline.x + diameter / 2 - 5, outline.y + diameter / 2 - 8,
           diameter <= 22 ? 12 : 14, kInk);
  DrawText(label, outline.x + diameter + 7,
           y + (layout_.bottom_bar_height - 14) / 2 - 2,
           layout_.bottom_bar_height <= 40 ? 12 : 14, kText, 58);
}

void RocShellApp::DrawText(const std::string &text, int x, int y, int point_size,
                           SDL_Color color, int max_width, bool right_align) {
  TTF_Font *font = Font(point_size);
  if (!font || text.empty()) return;
  const std::string display = max_width > 0 ? Ellipsize(text, font, max_width) : text;
  const std::string key = CacheKey(display, point_size, color);
  auto found = text_cache_.find(key);
  if (found == text_cache_.end()) {
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, display.c_str(), color);
    if (!surface) return;
    TextTexture value;
    value.texture = SDL_CreateTextureFromSurface(renderer_, surface);
    value.width = surface->w;
    value.height = surface->h;
    SDL_FreeSurface(surface);
    found = text_cache_.emplace(key, value).first;
  }
  if (!found->second.texture) return;
  SDL_Rect destination{right_align ? x - found->second.width : x, y,
                       found->second.width, found->second.height};
  SDL_RenderCopy(renderer_, found->second.texture, nullptr, &destination);
}

std::string RocShellApp::Ellipsize(const std::string &text, TTF_Font *font,
                                   int max_width) const {
  int width = 0;
  if (TTF_SizeUTF8(font, text.c_str(), &width, nullptr) == 0 && width <= max_width) {
    return text;
  }
  const std::string suffix = "...";
  std::vector<size_t> boundaries{0};
  for (size_t index = 0; index < text.size();) {
    const unsigned char byte = static_cast<unsigned char>(text[index]);
    size_t length = 1;
    if ((byte & 0xE0) == 0xC0) length = 2;
    else if ((byte & 0xF0) == 0xE0) length = 3;
    else if ((byte & 0xF8) == 0xF0) length = 4;
    index = std::min(text.size(), index + length);
    boundaries.push_back(index);
  }
  for (size_t count = boundaries.size(); count > 1; --count) {
    const std::string candidate = text.substr(0, boundaries[count - 2]) + suffix;
    if (TTF_SizeUTF8(font, candidate.c_str(), &width, nullptr) == 0 &&
        width <= max_width) {
      return candidate;
    }
  }
  return suffix;
}

TTF_Font *RocShellApp::Font(int point_size) {
  auto found = fonts_.find(point_size);
  if (found != fonts_.end()) return found->second;
  if (font_path_.empty()) return nullptr;
  TTF_Font *font = TTF_OpenFont(font_path_.c_str(), point_size);
  if (font) TTF_SetFontHinting(font, TTF_HINTING_LIGHT);
  fonts_[point_size] = font;
  return font;
}

SDL_Color RocShellApp::ColorForItem(const LibraryItem &item) const {
  return SDL_Color{item.red, item.green, item.blue, 255};
}
