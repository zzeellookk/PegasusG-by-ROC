#pragma once

#include "ui_types.h"

DisplayMetrics ResolveDisplayMetrics(int framebuffer_width, int framebuffer_height,
                                      double user_density = 0.0);
ResolvedLayout ResolveLayout(int viewport_width, int viewport_height, Insets safe_area = {},
                             double density = 1.0);
ResolvedLayout ResolveLayoutForDisplay(int framebuffer_width, int framebuffer_height,
                                       Insets safe_area = {}, double user_density = 0.0);
const char *LayoutModeName(LayoutMode mode);
