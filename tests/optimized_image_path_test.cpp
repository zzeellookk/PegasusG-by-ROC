#include "optimized_image_path.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

void Touch(const fs::path &path) {
  std::ofstream(path, std::ios::binary).put('\0');
}

void ExpectResolved(const fs::path &source, bool use_mini_assets,
                    const fs::path &expected, bool optimized) {
  const ResolvedImagePath resolved =
      ResolveOptimizedImagePath(source.u8string(), use_mini_assets);
  assert(fs::u8path(resolved.path) == expected);
  assert(resolved.optimized == optimized);
}

}  // namespace

int main() {
  const fs::path root = fs::temp_directory_path() / "pegasusg_optimized_image_path_test";
  std::error_code error;
  fs::remove_all(root, error);
  fs::create_directories(root);

  const fs::path png = root / "boxfront.png";
  Touch(png);
  Touch(root / "boxfront.H700.png");
  Touch(root / "mini_boxfront.jpg");
  Touch(root / "mini_boxfront.png");
  ExpectResolved(png, false, png, false);
  ExpectResolved(png, true, root / "mini_boxfront.png", true);

  fs::remove(root / "mini_boxfront.png");
  ExpectResolved(png, true, root / "mini_boxfront.jpg", true);

  const fs::path jpg = root / "logo.jpg";
  Touch(jpg);
  Touch(root / "mini_logo.png");
  Touch(root / "mini_logo.jpg");
  ExpectResolved(jpg, true, root / "mini_logo.jpg", true);

  const fs::path mixed_case = root / "boxFront.png";
  Touch(mixed_case);
  ExpectResolved(mixed_case, true, root / "mini_boxfront.jpg", true);

  const fs::path legacy_typo = root / "lgoo.png";
  Touch(legacy_typo);
  ExpectResolved(legacy_typo, true, root / "mini_logo.png", true);

  const fs::path unrelated = root / "screenshot.png";
  Touch(unrelated);
  Touch(root / "mini_screenshot.png");
  ExpectResolved(unrelated, true, unrelated, false);

  const fs::path old_h700_only = root / "old-cover.png";
  Touch(old_h700_only);
  Touch(root / "old-cover.H700.png");
  ExpectResolved(old_h700_only, true, old_h700_only, false);

  const fs::path original = root / "original.png";
  Touch(original);
  ExpectResolved(original, true, original, false);

  fs::remove_all(root, error);
  return 0;
}
