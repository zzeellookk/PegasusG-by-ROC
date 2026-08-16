#include "ui_draw.h"

#include <algorithm>
#include <cmath>

namespace roc_ui {

void SetColor(SDL_Renderer *renderer, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}

void Fill(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color) {
  SetColor(renderer, color);
  SDL_RenderFillRect(renderer, &rect);
}

void Stroke(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color) {
  SetColor(renderer, color);
  SDL_RenderDrawRect(renderer, &rect);
}

void StrokeRect(SDL_Renderer *renderer, int x, int y, int width, int height) {
  const SDL_Rect rect{x, y, width, height};
  SDL_RenderDrawRect(renderer, &rect);
}

void StrokeCircle(SDL_Renderer *renderer, int center_x, int center_y, int radius) {
  int x = radius;
  int y = 0;
  int error = 1 - radius;
  while (x >= y) {
    SDL_RenderDrawPoint(renderer, center_x + x, center_y + y);
    SDL_RenderDrawPoint(renderer, center_x + y, center_y + x);
    SDL_RenderDrawPoint(renderer, center_x - y, center_y + x);
    SDL_RenderDrawPoint(renderer, center_x - x, center_y + y);
    SDL_RenderDrawPoint(renderer, center_x - x, center_y - y);
    SDL_RenderDrawPoint(renderer, center_x - y, center_y - x);
    SDL_RenderDrawPoint(renderer, center_x + y, center_y - x);
    SDL_RenderDrawPoint(renderer, center_x + x, center_y - y);
    ++y;
    if (error < 0) {
      error += 2 * y + 1;
    } else {
      --x;
      error += 2 * (y - x) + 1;
    }
  }
}

void FillCircle(SDL_Renderer *renderer, int center_x, int center_y, int radius,
                SDL_Color color) {
  SetColor(renderer, color);
  for (int y = -radius; y <= radius; ++y) {
    const int half_width = static_cast<int>(std::sqrt(radius * radius - y * y));
    SDL_RenderDrawLine(renderer, center_x - half_width, center_y + y,
                       center_x + half_width, center_y + y);
  }
}

void FillRoundedRect(SDL_Renderer *renderer, const SDL_Rect &rect, int radius,
                     SDL_Color color) {
  radius = std::clamp(radius, 0, std::min(rect.w, rect.h) / 2);
  if (radius == 0) {
    Fill(renderer, rect, color);
    return;
  }
  Fill(renderer, SDL_Rect{rect.x + radius, rect.y, rect.w - radius * 2, rect.h},
       color);
  Fill(renderer, SDL_Rect{rect.x, rect.y + radius, rect.w, rect.h - radius * 2},
       color);
  FillCircle(renderer, rect.x + radius, rect.y + radius, radius, color);
  FillCircle(renderer, rect.x + rect.w - radius - 1, rect.y + radius, radius, color);
  FillCircle(renderer, rect.x + radius, rect.y + rect.h - radius - 1, radius, color);
  FillCircle(renderer, rect.x + rect.w - radius - 1,
             rect.y + rect.h - radius - 1, radius, color);
}

SDL_Rect Inset(SDL_Rect rect, int amount) {
  rect.x += amount;
  rect.y += amount;
  rect.w = std::max(0, rect.w - amount * 2);
  rect.h = std::max(0, rect.h - amount * 2);
  return rect;
}

}  // namespace roc_ui

