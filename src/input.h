#pragma once

#include <SDL.h>

#include "ui_types.h"

class InputRouter {
 public:
  InputRouter();
  ~InputRouter();

  UiAction Translate(const SDL_Event &event);

 private:
  SDL_GameController *controller_ = nullptr;
  int horizontal_axis_direction_ = 0;
  int vertical_axis_direction_ = 0;
};

