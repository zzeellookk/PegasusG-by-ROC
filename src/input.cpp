#include "input.h"

#include <cmath>

InputRouter::InputRouter() {
  for (int index = 0; index < SDL_NumJoysticks(); ++index) {
    if (!SDL_IsGameController(index)) continue;
    controller_ = SDL_GameControllerOpen(index);
    if (controller_) break;
  }
}

InputRouter::~InputRouter() {
  if (controller_) SDL_GameControllerClose(controller_);
}

UiAction InputRouter::Translate(const SDL_Event &event) {
  if (event.type == SDL_KEYDOWN) {
    switch (event.key.keysym.sym) {
      case SDLK_UP: return UiAction::Up;
      case SDLK_DOWN: return UiAction::Down;
      case SDLK_LEFT: return UiAction::Left;
      case SDLK_RIGHT: return UiAction::Right;
      case SDLK_RETURN:
      case SDLK_SPACE: return UiAction::Confirm;
      case SDLK_ESCAPE:
      case SDLK_BACKSPACE: return UiAction::Back;
      case SDLK_x: return UiAction::ContextPrimary;
      case SDLK_y: return UiAction::ContextSecondary;
      case SDLK_q:
      case SDLK_LEFTBRACKET: return UiAction::TabPrevious;
      case SDLK_e:
      case SDLK_RIGHTBRACKET: return UiAction::TabNext;
      case SDLK_PAGEUP: return UiAction::PagePrevious;
      case SDLK_PAGEDOWN: return UiAction::PageNext;
      case SDLK_m: return UiAction::Menu;
      case SDLK_f: return UiAction::Search;
      default: break;
    }
  }

  if (event.type == SDL_CONTROLLERBUTTONDOWN) {
    switch (event.cbutton.button) {
      case SDL_CONTROLLER_BUTTON_DPAD_UP: return UiAction::Up;
      case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return UiAction::Down;
      case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return UiAction::Left;
      case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return UiAction::Right;
      case SDL_CONTROLLER_BUTTON_A: return UiAction::Confirm;
      case SDL_CONTROLLER_BUTTON_B: return UiAction::Back;
      case SDL_CONTROLLER_BUTTON_X: return UiAction::ContextPrimary;
      case SDL_CONTROLLER_BUTTON_Y: return UiAction::ContextSecondary;
      case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return UiAction::TabPrevious;
      case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return UiAction::TabNext;
      case SDL_CONTROLLER_BUTTON_START: return UiAction::Menu;
      case SDL_CONTROLLER_BUTTON_BACK: return UiAction::Search;
      default: break;
    }
  }

  if (event.type == SDL_CONTROLLERAXISMOTION &&
      (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ||
       event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)) {
    constexpr int kThreshold = 17000;
    const int value = event.caxis.value;
    const int direction = value > kThreshold ? 1 : (value < -kThreshold ? -1 : 0);
    if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
      if (direction != 0 && direction != horizontal_axis_direction_) {
        horizontal_axis_direction_ = direction;
        return direction < 0 ? UiAction::Left : UiAction::Right;
      }
      if (direction == 0) horizontal_axis_direction_ = 0;
    } else {
      if (direction != 0 && direction != vertical_axis_direction_) {
        vertical_axis_direction_ = direction;
        return direction < 0 ? UiAction::Up : UiAction::Down;
      }
      if (direction == 0) vertical_axis_direction_ = 0;
    }
  }

  return UiAction::None;
}

