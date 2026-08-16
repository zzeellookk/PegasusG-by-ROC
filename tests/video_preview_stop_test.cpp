#include "video_preview.h"

#include <cassert>
#include <chrono>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

int main() {
#ifndef _WIN32
  alarm(3);
#endif
  VideoPreview preview;
  preview.StartVideo("blocked-preview.mp4", 1, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  const auto started = std::chrono::steady_clock::now();
  preview.StopVideo();
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - started);
  assert(elapsed < std::chrono::milliseconds(1000));

#ifndef _WIN32
  alarm(0);
#endif
  return 0;
}
