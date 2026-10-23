// Flat, bold graphics drawn from InkView primitives (no bitmaps). UI only.
#pragma once
#include "grid.h"
#include "inkview.h"

namespace gfx {

// Draws one board tile (background, bevel/outline and icon) into the square at (px,py) of size s.
//  depth     current floor; deeper floors draw fiercer monsters
//  entrance  this tile is where the hero started (draws the hero)
//  tappable  hidden tile that can be revealed right now
//  locked    hidden tile locked by an adjacent monster (drawn with an X)
void drawTile(const inkdelve::Tile &t, int px, int py, int s, ifont *font, int depth, bool entrance,
              bool tappable, bool locked);

// HUD icons, centered on (cx,cy), roughly 60px across.
void drawHeart(int cx, int cy);
void drawCoin(int cx, int cy);
void drawDepth(int cx, int cy);
void drawSword(int cx, int cy);

// Horizontal HP bar with a black outline; the filled part is red.
void drawHpBar(int x, int y, int w, int h, int hp, int maxHp);

// Black rectangular outline `t` pixels thick, drawn inside (x,y,w,h).
void outline(int x, int y, int w, int h, int t, int color);

}  // namespace gfx
