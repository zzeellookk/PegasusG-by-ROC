#include "pegasus_metadata.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main() {
  const fs::path root = fs::temp_directory_path() / "roc_pegasus_brick_scan_test";
  fs::remove_all(root);
  const fs::path gba = root / "GBA";
  const fs::path hack = root / "GBA hack";
  const fs::path vib = root / "GBA vib";
  fs::create_directories(gba);
  fs::create_directories(hack);
  fs::create_directories(vib);

  std::ofstream(gba / "normal.zip") << "rom";
  std::ofstream(hack / "mod.zip") << "rom";
  std::ofstream(vib / "feedback.zip") << "rom";
  {
    std::ofstream metadata(gba / "metadata.pegasus.txt");
    metadata << "collection: GBA\n\n";
    metadata << "game: Normal\nfile: normal.zip\nsort-by: 001\n";
  }

  const PegasusScanReport report = ScanPegasusGbaRoots(
      {gba.u8string(), hack.u8string(), vib.u8string()});
  assert(report.metadata_files == 1);
  assert(report.games.size() == 3);

  int standard = 0;
  int mods = 0;
  int rumble = 0;
  for (const GbaGame &game : report.games) {
    if (game.is_rumble) {
      ++rumble;
      assert(game.default_core == GbaCore::Gpsp);
    } else if (game.is_mod) {
      ++mods;
    } else {
      ++standard;
    }
  }
  assert(standard == 1);
  assert(mods == 1);
  assert(rumble == 1);

  fs::remove_all(root);
  return 0;
}
