#include "h700_services.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>

int main() {
  namespace fs = std::filesystem;
  const fs::path root = fs::temp_directory_path() / "pegasusg-brick-services-test";
  const fs::path hook = root / "zz_pegasusg_autostart.sh";
  std::error_code error;
  fs::remove_all(root, error);
  fs::create_directories(root);
  setenv("PEGASUSG_BRICK_AUTOSTART_HOOK", hook.c_str(), 1);

  H700Services services("/unused", (root / "state").string());
  assert(!services.AutostartEnabled());
  assert(!services.ReadStatus().autostart);
  std::ofstream(hook) << "#!/bin/sh\n";
  assert(services.AutostartEnabled());
  assert(services.ReadStatus().autostart);

  fs::remove_all(root, error);
  return 0;
}
