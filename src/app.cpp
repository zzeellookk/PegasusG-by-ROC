#include "app.h"

#include <SDL_image.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <utility>

namespace {

SDL_Texture *LoadCircularTexture(SDL_Renderer *renderer, const std::string &path,
                                 int texture_size) {
  SDL_Surface *loaded = IMG_Load(path.c_str());
  if (!loaded) return nullptr;

  const int source_side = std::min(loaded->w, loaded->h);
  SDL_Rect source{(loaded->w - source_side) / 2, (loaded->h - source_side) / 2,
                  source_side, source_side};
  SDL_Surface *scaled =
      SDL_CreateRGBSurfaceWithFormat(0, texture_size, texture_size, 32,
                                     SDL_PIXELFORMAT_ARGB8888);
  if (!scaled) {
    SDL_FreeSurface(loaded);
    return nullptr;
  }

  SDL_SetSurfaceBlendMode(loaded, SDL_BLENDMODE_NONE);
  SDL_BlitScaled(loaded, &source, scaled, nullptr);
  SDL_FreeSurface(loaded);

  if (SDL_MUSTLOCK(scaled)) SDL_LockSurface(scaled);
  const double center = (texture_size - 1) / 2.0;
  const double radius = texture_size / 2.0 - 1.0;
  for (int y = 0; y < texture_size; ++y) {
    Uint32 *row = reinterpret_cast<Uint32 *>(
        static_cast<Uint8 *>(scaled->pixels) + y * scaled->pitch);
    for (int x = 0; x < texture_size; ++x) {
      Uint8 red = 0;
      Uint8 green = 0;
      Uint8 blue = 0;
      Uint8 alpha = 0;
      SDL_GetRGBA(row[x], scaled->format, &red, &green, &blue, &alpha);

      const double dx = x - center;
      const double dy = y - center;
      const double distance = std::sqrt(dx * dx + dy * dy);
      if (distance > radius) {
        alpha = 0;
      } else if (distance > radius - 1.0) {
        alpha = static_cast<Uint8>(
            std::clamp((radius - distance) * static_cast<double>(alpha), 0.0, 255.0));
      }
      row[x] = SDL_MapRGBA(scaled->format, red, green, blue, alpha);
    }
  }
  if (SDL_MUSTLOCK(scaled)) SDL_UnlockSurface(scaled);

  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, scaled);
  SDL_FreeSurface(scaled);
  if (texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  return texture;
}

}  // namespace

RocShellApp::RocShellApp(AppOptions options) : options_(std::move(options)) {
  modules_ = {
      {"home", "主页", "最近使用", ModuleIcon::Home},
      {"novel", "小说", "小说库", ModuleIcon::Novel},
      {"comic", "漫画", "漫画库", ModuleIcon::Comic},
      {"video", "视频", "视频库", ModuleIcon::Video},
      {"music", "音乐", "音乐库", ModuleIcon::Music},
      {"ons", "ONS 游戏", "ONS 游戏库", ModuleIcon::Ons},
      {"krkr", "KRKR 游戏", "KRKR 游戏库", ModuleIcon::Krkr},
      {"settings", "设置", "系统与前端设置", ModuleIcon::Settings},
      {"power", "电源", "电源选项", ModuleIcon::Power},
  };
  catalog_ = LibraryCatalog::Demo(modules_);
  if (!modules_.empty()) {
    if (options_.initial_novel_source == NovelSourceMode::Online) {
      novel_source_mode_ = NovelSourceMode::Online;
    }
    selected_source_tab_ = novel_source_mode_ == NovelSourceMode::Online ? 1 : 0;
    selected_online_action_ = 0;
  }
  search_query_ = options_.search_query;
  selected_sidebar_ = std::clamp(options_.initial_module, 0,
                                 static_cast<int>(modules_.size()) - 1);
  active_module_ = selected_sidebar_;
  selected_card_ = std::max(0, options_.initial_card);
  focus_zone_ = options_.initial_sidebar ? FocusZone::Sidebar : FocusZone::Grid;
  if (options_.initial_source_focus && active_module_ >= 0 &&
      active_module_ < static_cast<int>(modules_.size()) &&
      modules_[active_module_].id == "novel") {
    focus_zone_ = FocusZone::Sources;
  }
  online_book_menu_open_ = options_.initial_online_book_menu;
  RefreshVisibleItems();
  ClampSelection();
}

RocShellApp::~RocShellApp() {
  scan_cancel_ = true;
  if (scan_thread_.joinable()) scan_thread_.join();
  for (auto &[key, value] : text_cache_) {
    (void)key;
    if (value.texture) SDL_DestroyTexture(value.texture);
  }
  for (auto &[size, font] : fonts_) {
    (void)size;
    if (font) TTF_CloseFont(font);
  }
  delete input_;
  if (avatar_texture_) SDL_DestroyTexture(avatar_texture_);
  if (cover_placeholder_) SDL_DestroyTexture(cover_placeholder_);
  if (renderer_) SDL_DestroyRenderer(renderer_);
  if (window_) SDL_DestroyWindow(window_);
  if (initialized_) {
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
  }
}

bool RocShellApp::Initialize() {
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
    return false;
  }
  if ((IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & IMG_INIT_PNG) == 0) {
    std::cerr << "IMG_Init failed: " << IMG_GetError() << '\n';
    return false;
  }
  if (TTF_Init() != 0) {
    std::cerr << "TTF_Init failed: " << TTF_GetError() << '\n';
    return false;
  }
  initialized_ = true;

  const Uint32 window_flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE |
                              (options_.hidden ? SDL_WINDOW_HIDDEN
                                               : SDL_WINDOW_SHOWN);
  window_ = SDL_CreateWindow("ROC Shell - Adaptive UI Prototype",
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             options_.width, options_.height, window_flags);
  if (!window_) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
    return false;
  }

  renderer_ = SDL_CreateRenderer(
      window_, -1, options_.hidden ? SDL_RENDERER_SOFTWARE
                                   : SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer_) renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
  if (!renderer_) {
    std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
    return false;
  }
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

  font_path_ = FindFile({
      "assets/fonts/ui_font_02.ttf",
      "fonts/ui_font_02.ttf",
      "../assets/fonts/ui_font_02.ttf",
      "C:/Windows/Fonts/msyh.ttc",
  });
  if (font_path_.empty() || !Font(18)) {
    std::cerr << "No usable CJK font found\n";
    return false;
  }

  LoadOnlineSources();

  const std::string cover_path = FindFile({
      "assets/placeholders/default_cover.png",
  });
  if (!cover_path.empty()) cover_placeholder_ = IMG_LoadTexture(renderer_, cover_path.c_str());

  std::string avatar_path = options_.avatar_path;
  if (avatar_path.empty()) {
    avatar_path = FindFile({
        "data/conf/avatar.png",
        "data/conf/avatar.jpg",
        "data/conf/avatar.jpeg",
        "assets/config/avatar.png",
        "assets/config/avatar.jpg",
        "assets/config/avatar.jpeg",
        "avatar.png",
        "avatar.jpg",
        "avatar.jpeg",
    });
  }
  if (!avatar_path.empty()) {
    avatar_texture_ = LoadCircularTexture(renderer_, avatar_path, 160);
    if (!avatar_texture_) {
      std::cerr << "Avatar image ignored: " << avatar_path << " (" << IMG_GetError()
                << ")\n";
    }
  }

  input_ = new InputRouter();
  RecalculateLayout();
  RefreshVisibleItems();
  ClampSelection();
  StartLibraryScan();
  return true;
}

int RocShellApp::Run() {
  int rendered_frames = 0;
  while (running_) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running_ = false;
      if (event.type == SDL_WINDOWEVENT &&
          (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
           event.window.event == SDL_WINDOWEVENT_RESIZED)) {
        RecalculateLayout();
      }
      if (HandleTextInput(event)) continue;
      const UiAction action = input_->Translate(event);
      if (action != UiAction::None) HandleAction(action);
    }

    PollLibraryScan();
    Render();
    SDL_RenderPresent(renderer_);
    ++rendered_frames;

    if (!options_.screenshot_path.empty() && !screenshot_written_ && rendered_frames >= 2) {
      screenshot_written_ = SaveScreenshot(options_.screenshot_path);
      running_ = false;
    }
    if (options_.hidden && options_.screenshot_path.empty()) running_ = false;
  }
  return options_.screenshot_path.empty() || screenshot_written_ ? 0 : 2;
}

bool RocShellApp::SaveScreenshot(const std::string &path) {
  int width = 0;
  int height = 0;
  SDL_GetRendererOutputSize(renderer_, &width, &height);
  SDL_Surface *surface =
      SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
  if (!surface) return false;
  const bool read_ok = SDL_RenderReadPixels(renderer_, nullptr, SDL_PIXELFORMAT_ARGB8888,
                                            surface->pixels, surface->pitch) == 0;
  const bool save_ok = read_ok && IMG_SavePNG(surface, path.c_str()) == 0;
  SDL_FreeSurface(surface);
  if (!save_ok) {
    std::cerr << "Screenshot failed: " << SDL_GetError() << ' ' << IMG_GetError() << '\n';
  }
  return save_ok;
}

std::string RocShellApp::FindFile(const std::vector<std::string> &candidates) const {
  for (const std::string &candidate : candidates) {
    std::error_code error;
    if (std::filesystem::is_regular_file(std::filesystem::u8path(candidate), error)) {
      return candidate;
    }
  }
  return {};
}
