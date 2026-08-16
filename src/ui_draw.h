#pragma once

#include <SDL.h>

namespace roc_ui {

inline constexpr SDL_Color kBackground{246, 247, 249, 255};
inline constexpr SDL_Color kTopBar{255, 255, 255, 255};
inline constexpr SDL_Color kSidebar{255, 255, 255, 255};
inline constexpr SDL_Color kSurface{255, 255, 255, 255};
inline constexpr SDL_Color kSurfaceSelected{255, 240, 246, 255};
inline constexpr SDL_Color kText{24, 25, 28, 255};
inline constexpr SDL_Color kMuted{96, 102, 110, 255};
inline constexpr SDL_Color kDim{148, 153, 160, 255};
inline constexpr SDL_Color kAccent{251, 114, 153, 255};
inline constexpr SDL_Color kFocus{0, 174, 236, 255};
inline constexpr SDL_Color kDivider{227, 229, 231, 255};
inline constexpr SDL_Color kInk{255, 255, 255, 255};

void SetColor(SDL_Renderer *renderer, SDL_Color color);
void Fill(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color);
void Stroke(SDL_Renderer *renderer, const SDL_Rect &rect, SDL_Color color);
void StrokeRect(SDL_Renderer *renderer, int x, int y, int width, int height);
void StrokeCircle(SDL_Renderer *renderer, int center_x, int center_y, int radius);
void FillCircle(SDL_Renderer *renderer, int center_x, int center_y, int radius,
                SDL_Color color);
void FillRoundedRect(SDL_Renderer *renderer, const SDL_Rect &rect, int radius,
                     SDL_Color color);
SDL_Rect Inset(SDL_Rect rect, int amount);

}  // namespace roc_ui

