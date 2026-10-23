// Flat, bold icons drawn from InkView primitives (no bitmaps). UI only.
#pragma once
#include "grid.h"
#include "inkview.h"

namespace gfx {

// Draws the face of a revealed tile (icon plus its number) into the square at (px,py) of size s.
// Hidden and empty tiles are handled by the caller.
void drawTileIcon(const inkdelve::Tile &t, int px, int py, int s, ifont *font);

// Bold white X for a hidden tile locked by an adjacent monster.
void drawX(int px, int py, int s);

// Small HUD icons, centered on (cx,cy), roughly 60px across.
void drawHeart(int cx, int cy);
void drawCoin(int cx, int cy);
void drawDepth(int cx, int cy);

}  // namespace gfx
