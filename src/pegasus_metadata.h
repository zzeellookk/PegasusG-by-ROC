#pragma once

#include <string>
#include <vector>

#include "gba_model.h"

PegasusScanReport ScanPegasusGbaRoots(const std::vector<std::string> &roots,
                                      const std::string &mod_overrides_path = {});

bool LooksLikeGbaMod(const GbaGame &game);
bool LooksLikeGbaRumble(const GbaGame &game);
