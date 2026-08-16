#include "pegasus_metadata.h"

#include <cassert>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main() {
  const fs::path root = fs::temp_directory_path() / "roc_pegasus_metadata_test";
  fs::remove_all(root);
  const fs::path gba = root / "GBA";
  const fs::path hack = root / "GBA hack";
  const fs::path vib = root / "GBA vib";
  fs::create_directories(gba / "media" / "普通游戏");
  fs::create_directories(hack / "media" / "特别版本");
  fs::create_directories(vib);
  std::ofstream(gba / "normal.zip") << "rom";
  std::ofstream(gba / "normal-alt.zip") << "rom";
  std::ofstream(gba / "名字带震动.zip") << "rom";
  std::ofstream(gba / "vbam.zip") << "rom";
  std::ofstream(gba / "vba-next.zip") << "rom";
  std::ofstream(gba / "unused.zip") << "rom";
  std::ofstream(hack / "special.zip") << "rom";
  std::ofstream(vib / "feedback.zip") << "rom";
  std::ofstream(vib / "feedback-mgba.zip") << "rom";
  std::ofstream(gba / "media" / "普通游戏" / "boxfront.png") << "png";
  std::ofstream(hack / "media" / "特别版本" / "video.mp4") << "mp4";
  {
    std::ofstream metadata(gba / "metadata.pegasus.txt");
    metadata << "collection: GBA\nlaunch: ignored android command\n\n";
    metadata << "game: 普通游戏\nfiles:\n  normal.zip\n  normal-alt.zip\nsort-by: 001\n";
    metadata << "launch:\n  -e LIBRETRO /cores/gpsp_libretro_android.so\n";
    metadata << "developer: Test\ndescription: 第一行\\n第二行\n\n";
    metadata << "game: 名字带震动\nfile: 名字带震动.zip\nsort-by: 002\n";
    metadata << "developer: Test\ndescription: Still standard\n\n";
    metadata << "game: VBA-M游戏\nfile: vbam.zip\nsort-by: 0021\n";
    metadata << "launch:\n  -e LIBRETRO /cores/vbam_libretro_android.so\n\n";
    metadata << "game: VBA Next游戏\nfile: vba-next.zip\nsort-by: 0022\n";
    metadata << "launch:\n  -e LIBRETRO /cores/vba_next_libretro_android.so\n\n";
    metadata << "game: 缺失游戏\nfile: missing.zip\nsort-by: 005\n";
  }
  {
    std::ofstream metadata(hack / "metadata.pegasus.txt");
    metadata << "collection: GBA hack\n\n";
    metadata << "game: 特别版本\nfile: special.zip\nsort-by: 003\n";
    metadata << "launch:\n  -e LIBRETRO /cores/gpsp_libretro_android.so\n";
    metadata << "developer: Test\ndescription: Hack collection\n\n";
  }
  {
    std::ofstream metadata(vib / "metadata.pegasus.txt");
    metadata << "collection: GBA vib\n\n";
    metadata << "game: 反馈演示\nfile: feedback.zip\nsort-by: 004\n";
    metadata << "developer: Test\ndescription: Rumble collection\n\n";
    metadata << "game: mGBA震动游戏\nfile: feedback-mgba.zip\nsort-by: 005\n";
    metadata << "launch:\n  -e LIBRETRO /cores/mgba_libretro_android.so\n";
    metadata << "developer: Test\ndescription: Explicit mGBA in rumble collection\n\n";
  }

  const PegasusScanReport report = ScanPegasusGbaRoots({root.u8string()});
  assert(report.metadata_files == 3);
  assert(report.parsed_entries == 8);
  assert(report.games.size() == 7);
  assert(report.missing_rom_entries == 1);
  assert(report.unreferenced_roms == 1);
  assert(report.games[0].title == "普通游戏");
  assert(report.games[0].description == "第一行\n第二行");
  assert(!report.games[0].cover_path.empty());
  assert(!report.games[0].is_mod);
  assert(!report.games[0].is_rumble);
  assert(report.games[0].default_core == GbaCore::Gpsp);
  assert(report.games[0].core == GbaCore::Gpsp);
  assert(report.games[0].alternate_roms.size() == 1);
  assert(std::string(GbaLaunchCoreName(report.games[0])) == "gpsp");
  assert(report.games[1].title == "名字带震动");
  assert(!report.games[1].is_rumble);
  assert(report.games[2].title == "VBA-M游戏");
  assert(report.games[2].core == GbaCore::Vbam);
  assert(std::string(GbaLaunchCoreName(report.games[2])) == "vbam");
  assert(report.games[3].title == "VBA Next游戏");
  assert(report.games[3].core == GbaCore::VbaNext);
  assert(std::string(GbaLaunchCoreName(report.games[3])) == "vba_next");
  assert(report.games[4].title == "特别版本");
  assert(report.games[4].is_mod);
  assert(!report.games[4].video_path.empty());
  assert(report.games[4].core == GbaCore::Gpsp);
  assert(report.games[4].default_core == GbaCore::Gpsp);
  assert(std::string(GbaLaunchCoreName(report.games[4])) == "gpsp");
  assert(report.games[5].title == "反馈演示");
  assert(report.games[5].is_rumble);
  assert(report.games[5].core == GbaCore::Gpsp);
  assert(report.games[5].default_core == GbaCore::Gpsp);
  assert(std::string(GbaLaunchCoreName(report.games[5])) == "gpsp_rumble");
  assert(report.games[6].title == "mGBA震动游戏");
  assert(report.games[6].is_rumble);
  assert(report.games[6].core == GbaCore::Mgba);
  assert(report.games[6].default_core == GbaCore::Mgba);
  assert(std::string(GbaLaunchCoreName(report.games[6])) == "mgba");
  fs::remove_all(root);
  return 0;
}
