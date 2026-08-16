#define SDL_MAIN_HANDLED

#include "gba_frontend.h"
#include "library_scanner.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace {

std::string Env(const char *name, const std::string &fallback = {}) {
  const char *value = std::getenv(name);
  return value && *value ? value : fallback;
}

bool Flag(const char *name) {
  const std::string value = Env(name);
  return value == "1" || value == "true" || value == "yes";
}

int Positive(const char *value, int fallback) {
  if (!value) return fallback;
  int parsed = 0;
  const std::string text = value;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
  return result.ec == std::errc() && result.ptr == text.data() + text.size() && parsed > 0
      ? parsed : fallback;
}

int NonNegative(const char *value, int fallback) {
  if (!value) return fallback;
  int parsed = 0;
  const std::string text = value;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
  return result.ec == std::errc() && result.ptr == text.data() + text.size() && parsed >= 0
      ? parsed : fallback;
}

}  // namespace

int main(int argc, char **argv) {
  GbaFrontendOptions options;
  options.width = Positive(std::getenv("PEGASUSG_WIDTH"), 720);
  options.height = Positive(std::getenv("PEGASUSG_HEIGHT"), 480);
  options.app_dir = Env("PEGASUSG_APP_DIR", fs::current_path().u8string());
  options.state_dir = Env("PEGASUSG_STATE_DIR", (fs::u8path(options.app_dir) / "data").u8string());
  options.launch_request_path = Env("PEGASUSG_LAUNCH_REQUEST",
      (fs::u8path(options.state_dir) / "launch.request").u8string());
  options.mod_overrides_path = Env("PEGASUSG_MOD_OVERRIDES",
      (fs::u8path(options.app_dir) / "config/mods.txt").u8string());
  options.font_path = Env("PEGASUSG_FONT");
  options.diagnostics = Flag("PEGASUSG_DIAGNOSTICS");
  options.no_video = Flag("PEGASUSG_NO_VIDEO");
  std::string device = Env("PEGASUSG_DEVICE");
  std::transform(device.begin(), device.end(), device.begin(),
                 [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
  options.use_mini_assets = device == "h700";
  const std::string roots = Env("PEGASUSG_CONTENT_ROOTS", "/mnt/mmc/Roms/GBA:/mnt/sdcard/Roms/GBA");
  options.content_roots = SplitLibraryRootList(roots);

  for (int i = 1; i < argc; ++i) {
    const std::string argument = argv[i];
    if (argument == "--width" && i + 1 < argc) options.width = Positive(argv[++i], options.width);
    else if (argument == "--height" && i + 1 < argc) options.height = Positive(argv[++i], options.height);
    else if (argument == "--content-root" && i + 1 < argc) options.content_roots.push_back(argv[++i]);
    else if (argument == "--screenshot" && i + 1 < argc) { options.screenshot_path = argv[++i]; options.no_video = true; }
    else if (argument == "--screenshot-action" && i + 1 < argc) options.screenshot_action = argv[++i];
    else if (argument == "--screenshot-delay" && i + 1 < argc) {
      options.screenshot_delay_ms = NonNegative(argv[++i], options.screenshot_delay_ms);
    }
    else if (argument == "--no-video") options.no_video = true;
    else if (argument == "--diagnostics") options.diagnostics = true;
    else if (argument == "--autostart") options.restore_ui = false;
    else if (argument == "--restore-ui") options.restore_ui = true;
    else if (argument == "--reuse-bgm") options.reuse_bgm_track = true;
    else if (argument == "--help") {
      std::cout << "PegasusG by ROC\n"
                << "  --content-root PATH (repeatable)\n"
                << "  --width N --height N\n"
                << "  --screenshot PATH [--screenshot-action ACTION --screenshot-delay MS]\n"
                << "  --no-video --diagnostics\n"
                << "  --autostart --restore-ui\n";
      return 0;
    }
  }
  GbaFrontend app(std::move(options));
  if (!app.Initialize()) return 1;
  return app.Run();
}
