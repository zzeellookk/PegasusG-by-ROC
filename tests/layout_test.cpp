#include "layout.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

bool Nearly(double left, double right) {
  return std::abs(left - right) < 0.01;
}

}  // namespace

int main() {
  struct Case {
    int width;
    int height;
    int logical_width;
    int logical_height;
    double scale;
    double density;
  };
  const std::vector<Case> viewports = {
      {640, 480, 640, 480, 1.000, 1.00},
      {720, 480, 720, 480, 1.000, 1.00},
      {720, 720, 720, 720, 1.000, 1.00},
      {854, 480, 854, 480, 1.000, 1.00},
      {1024, 768, 960, 720, 1.067, 1.00},
      {1280, 720, 1280, 720, 1.000, 1.00},
      {1280, 800, 1152, 720, 1.111, 1.00},
      {1600, 1440, 800, 720, 2.000, 1.00},
  };

  for (const Case &item : viewports) {
    const DisplayMetrics metrics = ResolveDisplayMetrics(item.width, item.height);
    assert(metrics.framebuffer_width == item.width);
    assert(metrics.framebuffer_height == item.height);
    assert(metrics.logical_width == item.logical_width);
    assert(metrics.logical_height == item.logical_height);
    assert(Nearly(metrics.window_scale, item.scale));
    assert(Nearly(metrics.density, item.density));

    const ResolvedLayout layout = ResolveLayoutForDisplay(item.width, item.height);
    assert(layout.display.logical_width == metrics.logical_width);
    assert(layout.display.logical_height == metrics.logical_height);
    assert(layout.viewport_width == metrics.logical_width);
    assert(layout.viewport_height == metrics.logical_height);
    assert(layout.grid_columns >= 2 && layout.grid_columns <= 6);
    assert(layout.card_width > 0);
    assert(layout.card_cover_height > layout.card_width);
    assert(layout.grid_x >= layout.content_x);
    assert(layout.grid_x + layout.grid_width <= metrics.logical_width);
    assert(layout.grid_y + layout.grid_height + layout.bottom_bar_height <= metrics.logical_height);
    assert(layout.sidebar_width < metrics.logical_width / 2);
    assert(!layout.expanded_sidebar);
    assert(layout.content_x == layout.safe_area.left);
    assert(layout.content_width == metrics.logical_width - layout.safe_area.left -
                                       layout.safe_area.right);
    std::cout << item.width << 'x' << item.height
              << " logical=" << metrics.logical_width << 'x' << metrics.logical_height
              << " scale=" << metrics.window_scale
              << " density=" << metrics.density
              << ' ' << LayoutModeName(layout.mode)
              << " sidebar=" << layout.sidebar_width
              << " columns=" << layout.grid_columns
              << " card=" << layout.card_width << 'x' << layout.card_cover_height << '\n';
  }

  const DisplayMetrics custom_density = ResolveDisplayMetrics(640, 480, 1.0);
  assert(Nearly(custom_density.density, 1.0));

  const ResolvedLayout square_base = ResolveLayoutForDisplay(720, 720);
  const ResolvedLayout square_hidpi = ResolveLayoutForDisplay(1600, 1440);
  const ResolvedLayout handheld_3x2 = ResolveLayoutForDisplay(720, 480);
  const ResolvedLayout handheld_4x3 = ResolveLayoutForDisplay(640, 480);
  assert(square_base.mode == LayoutMode::Square);
  assert(square_hidpi.mode == LayoutMode::Square);
  assert(square_base.grid_columns == 4);
  assert(square_hidpi.grid_columns == 4);
  assert(square_hidpi.display.window_scale == 2.0);
  assert(Nearly(square_hidpi.display.density, 1.0));
  assert(handheld_3x2.display.logical_width == 720);
  assert(handheld_3x2.display.logical_height == 480);
  assert(handheld_3x2.display.window_scale == 1.0);
  assert(handheld_3x2.grid_columns == 4);
  assert(handheld_4x3.display.logical_width == 640);
  assert(handheld_4x3.display.logical_height == 480);
  assert(handheld_4x3.grid_columns == 4);

  const ResolvedLayout safe = ResolveLayoutForDisplay(720, 480, Insets{10, 12, 14, 16});
  assert(safe.safe_area.left == 10);
  assert(safe.safe_area.top == 12);
  assert(safe.safe_area.right == 14);
  assert(safe.safe_area.bottom == 16);
  assert(safe.content_width == safe.viewport_width - 24);
  return 0;
}
