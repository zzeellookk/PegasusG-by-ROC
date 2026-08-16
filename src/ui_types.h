#pragma once

#include <string>
#include <vector>

enum class LayoutMode {
  Compact,
  Standard,
  Wide,
  Square,
  Tall,
};

enum class UiAction {
  None,
  Up,
  Down,
  Left,
  Right,
  Confirm,
  Back,
  ContextPrimary,
  ContextSecondary,
  TabPrevious,
  TabNext,
  PagePrevious,
  PageNext,
  Menu,
  Search,
};

enum class FocusZone {
  Sidebar,
  Sources,
  Filters,
  Grid,
  OnlineBookMenu,
};

enum class ModuleIcon {
  Home,
  Novel,
  Comic,
  Video,
  Music,
  Ons,
  Krkr,
  Settings,
  Power,
};

struct Insets {
  int left = 0;
  int top = 0;
  int right = 0;
  int bottom = 0;
};

struct DisplayMetrics {
  int framebuffer_width = 0;
  int framebuffer_height = 0;
  int logical_width = 0;
  int logical_height = 720;
  double window_scale = 1.0;
  double density = 1.0;
};

struct ModuleDefinition {
  std::string id;
  std::string title;
  std::string section_title;
  ModuleIcon icon = ModuleIcon::Home;
};

struct LibraryItem {
  std::string id;
  std::string module_id;
  std::string title;
  std::string metadata;
  std::string path;
  std::string source_id;
  std::string source_url;
  bool favorite = false;
  int recent_order = 0;
  int progress_percent = -1;
  unsigned char red = 70;
  unsigned char green = 100;
  unsigned char blue = 130;
};

struct ResolvedLayout {
  int viewport_width = 0;
  int viewport_height = 0;
  DisplayMetrics display;
  LayoutMode mode = LayoutMode::Standard;
  Insets safe_area;

  bool expanded_sidebar = false;
  int top_bar_height = 0;
  int bottom_bar_height = 0;
  int sidebar_width = 0;
  int sidebar_item_height = 0;
  int filter_bar_height = 0;
  int section_header_height = 0;
  int content_padding = 0;
  int grid_gap = 0;
  int card_footer_height = 0;
  int card_width = 0;
  int card_cover_height = 0;
  int grid_columns = 0;

  int content_x = 0;
  int content_width = 0;
  int grid_x = 0;
  int grid_y = 0;
  int grid_width = 0;
  int grid_height = 0;
};
