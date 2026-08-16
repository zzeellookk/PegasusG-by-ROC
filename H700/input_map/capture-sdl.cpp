#include <SDL.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

int main() {
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
    std::cerr << "ERROR=SDL_Init:" << SDL_GetError() << '\n';
    return 1;
  }

  std::vector<SDL_GameController *> controllers;
  std::vector<SDL_Joystick *> joysticks;
  for (int index = 0; index < SDL_NumJoysticks(); ++index) {
    if (SDL_IsGameController(index)) {
      if (SDL_GameController *controller = SDL_GameControllerOpen(index)) {
        controllers.push_back(controller);
      }
    } else if (SDL_Joystick *joystick = SDL_JoystickOpen(index)) {
      joysticks.push_back(joystick);
    }
  }
  SDL_GameControllerEventState(SDL_ENABLE);
  SDL_JoystickEventState(SDL_ENABLE);
  std::cout << "READY=SDL_MENU controllers=" << controllers.size()
            << " joysticks=" << SDL_NumJoysticks() << std::endl;

  bool captured = false;
  Uint32 captured_at = 0;
  const Uint32 deadline = SDL_GetTicks() + 30000;
  while (!SDL_TICKS_PASSED(SDL_GetTicks(), deadline)) {
    SDL_Event event{};
    if (!SDL_WaitEventTimeout(&event, 50)) {
      if (captured && SDL_GetTicks() - captured_at >= 500) break;
      continue;
    }

    bool interesting = false;
    if (event.type == SDL_CONTROLLERBUTTONDOWN) {
      const auto button = static_cast<SDL_GameControllerButton>(event.cbutton.button);
      const char *name = SDL_GameControllerGetStringForButton(button);
      std::cout << "CONTROLLER_BUTTON button=" << static_cast<int>(event.cbutton.button)
                << " name=" << (name ? name : "unknown") << std::endl;
      interesting = true;
    } else if (event.type == SDL_JOYBUTTONDOWN) {
      std::cout << "JOY_BUTTON button=" << static_cast<int>(event.jbutton.button)
                << std::endl;
      interesting = true;
    } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
      std::cout << "KEY_DOWN scancode=" << static_cast<int>(event.key.keysym.scancode)
                << " keycode=" << static_cast<int>(event.key.keysym.sym)
                << " name=" << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
      interesting = true;
    } else if (event.type == SDL_JOYHATMOTION && event.jhat.value != SDL_HAT_CENTERED) {
      std::cout << "JOY_HAT hat=" << static_cast<int>(event.jhat.hat)
                << " value=" << static_cast<int>(event.jhat.value) << std::endl;
      interesting = true;
    }

    if (interesting) {
      captured = true;
      captured_at = SDL_GetTicks();
    }
  }

  for (SDL_GameController *controller : controllers) SDL_GameControllerClose(controller);
  for (SDL_Joystick *joystick : joysticks) SDL_JoystickClose(joystick);
  SDL_Quit();
  if (!captured) {
    std::cout << "TIMEOUT=SDL_MENU" << std::endl;
    return 2;
  }
  return 0;
}
