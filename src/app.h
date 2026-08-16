#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "cover_grid_data_source.h"
#include "input.h"
#include "layout.h"
#include "library_catalog.h"
#include "library_scanner.h"
#include "online_sources.h"
#include "ui_types.h"

enum class NovelSourceMode {
  Local,
  Online,
};

struct AppOptions {
  int width = 1280;
  int height = 800;
  double density = 0.0;
  Insets safe_area;
  bool hidden = false;
  bool initial_sidebar = false;
  bool diagnostics = false;
  std::string screenshot_path;
  std::string search_query;
  std::string avatar_path;
  std::string online_sources_config;
  std::vector<std::string> scan_roots;
  int max_scan_items = 10000;
  int initial_module = 1;
  int initial_card = 0;
  NovelSourceMode initial_novel_source = NovelSourceMode::Local;
  bool initial_source_focus = false;
  bool initial_online_book_menu = false;
};

class RocShellApp {
 public:
  explicit RocShellApp(AppOptions options);
  ~RocShellApp();

  bool Initialize();
  int Run();

 private:
  struct TextTexture {
    SDL_Texture *texture = nullptr;
    int width = 0;
    int height = 0;
  };

  bool HandleAction(UiAction action);
  bool HandleTextInput(const SDL_Event &event);
  bool IsNovelModule() const;
  bool IsSettingsModule() const;
  bool IsOnlineNovelMode() const;
  bool HasNovelSourceSwitch() const;
  const LibraryCatalog &ActiveCatalog() const;
  LibraryCatalog &ActiveCatalog();
  void SetNovelSourceMode(NovelSourceMode mode);
  void LoadOnlineSources();
  void RebuildOnlineCatalog();
  void RebuildSettingsCatalog();
  void SelectOnlineSource(int index);
  std::string ActiveSourceName() const;
  void RecalculateLayout();
  void RefreshVisibleItems();
  int CurrentItemCount() const;
  int SelectedLibraryItemIndex() const;
  void ClampSelection();
  void StartLibraryScan();
  void PollLibraryScan();
  void SetStatus(std::string message, Uint32 duration_ms = 1800);
  void LogDiagnostics(const char *reason) const;
  void Render();
  void RenderTopBar();
  void RenderAvatar(const SDL_Rect &bounds);
  void RenderSidebar();
  void RenderFilterBar();
  void RenderLibrary();
  void RenderNovelSourceSwitch(int header_y);
  void RenderOnlineBookMenu();
  void RenderBottomBar();
  void RenderToast();
  void RenderIcon(ModuleIcon icon, const SDL_Rect &bounds, SDL_Color color);
  void RenderButtonHint(int x, int y, const char *button, const char *label);
  void DrawText(const std::string &text, int x, int y, int point_size, SDL_Color color,
                int max_width = 0, bool right_align = false);
  std::string Ellipsize(const std::string &text, TTF_Font *font, int max_width) const;
  TTF_Font *Font(int point_size);
  SDL_Color ColorForItem(const LibraryItem &item) const;
  bool SaveScreenshot(const std::string &path);
  std::string FindFile(const std::vector<std::string> &candidates) const;

  AppOptions options_;
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *cover_placeholder_ = nullptr;
  SDL_Texture *avatar_texture_ = nullptr;
  std::string font_path_;
  bool initialized_ = false;
  bool running_ = true;
  bool screenshot_written_ = false;

  InputRouter *input_ = nullptr;
  DisplayMetrics display_;
  ResolvedLayout layout_;
  FocusZone focus_zone_ = FocusZone::Grid;
  int selected_sidebar_ = 1;
  int active_module_ = 1;
  int selected_filter_ = 0;
  int selected_card_ = 0;
  NovelSourceMode novel_source_mode_ = NovelSourceMode::Local;
  int selected_source_tab_ = 0;
  int selected_online_action_ = 0;
  bool online_book_menu_open_ = false;
  int scroll_row_ = 0;
  bool search_active_ = false;
  std::string search_query_;
  std::string status_message_;
  Uint32 status_until_ = 0;

  std::vector<ModuleDefinition> modules_;
  LibraryCatalog catalog_;
  LibraryCatalog online_catalog_;
  OnlineSourceConfig online_sources_;
  CoverGridDataSource grid_data_;
  std::thread scan_thread_;
  std::atomic<bool> scan_cancel_{false};
  std::atomic<bool> scan_ready_{false};
  bool scan_running_ = false;
  std::vector<std::vector<LibraryItem>> scan_result_;
  std::unordered_map<int, TTF_Font *> fonts_;
  std::unordered_map<std::string, TextTexture> text_cache_;
};
