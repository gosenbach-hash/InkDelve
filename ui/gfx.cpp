#include "gfx.h"

#include <cmath>
#include <cstdio>

namespace gfx {

namespace {

const int kWhite = 0x00FFFFFF;
const int kBlack = 0x00000000;
const int kRed = 0x00CC0000;
const int kGoldDark = 0x00B07800;
const int kGoldLight = 0x00FFE066;
const int kPotionGreen = 0x00007700;
const int kCork = 0x00804A18;
const int kBlue = 0x000000CC;

int f(int s, double frac) { return static_cast<int>(frac * s + 0.5); }

void fillCircle(int cx, int cy, int r, int color) {
    for (int dy = -r; dy <= r; ++dy) {
        const int dx = static_cast<int>(std::sqrt(static_cast<double>(r * r - dy * dy)));
        FillArea(cx - dx, cy + dy, 2 * dx + 1, 1, color);
    }
}

// Scanline triangle fill.
void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) {
    const int xs[3] = {x1, x2, x3};
    const int ys[3] = {y1, y2, y3};
    int top = y1, bottom = y1;
    for (int i = 1; i < 3; ++i) {
        if (ys[i] < top) top = ys[i];
        if (ys[i] > bottom) bottom = ys[i];
    }
    for (int y = top; y <= bottom; ++y) {
        double lo = 1e9, hi = -1e9;
        for (int i = 0; i < 3; ++i) {
            const int j = (i + 1) % 3;
            if ((ys[i] <= y && y <= ys[j]) || (ys[j] <= y && y <= ys[i])) {
                if (ys[i] == ys[j]) {
                    if (xs[i] < lo) lo = xs[i];
                    if (xs[j] < lo) lo = xs[j];
                    if (xs[i] > hi) hi = xs[i];
                    if (xs[j] > hi) hi = xs[j];
                } else {
                    const double x = xs[i] + (xs[j] - xs[i]) * (static_cast<double>(y - ys[i]) / (ys[j] - ys[i]));
                    if (x < lo) lo = x;
                    if (x > hi) hi = x;
                }
            }
        }
        if (hi >= lo) FillArea(static_cast<int>(lo), y, static_cast<int>(hi - lo) + 1, 1, color);
    }
}

void drawNumber(const char *text, int px, int py, int s, ifont *font, int color) {
    SetFont(font, color);
    DrawTextRect(px, py + f(s, 0.68), s, f(s, 0.30), text, ALIGN_CENTER | VALIGN_MIDDLE);
}

void monster(int px, int py, int s, int hp, ifont *font) {
    // Skull: round head, jaw block, eyes, nose, teeth.
    fillCircle(px + f(s, 0.5), py + f(s, 0.36), f(s, 0.24), kWhite);
    FillArea(px + f(s, 0.34), py + f(s, 0.48), f(s, 0.32), f(s, 0.16), kWhite);
    fillCircle(px + f(s, 0.42), py + f(s, 0.34), f(s, 0.06), kBlack);
    fillCircle(px + f(s, 0.58), py + f(s, 0.34), f(s, 0.06), kBlack);
    fillTriangle(px + f(s, 0.47), py + f(s, 0.46), px + f(s, 0.53), py + f(s, 0.46),
                 px + f(s, 0.50), py + f(s, 0.41), kBlack);
    for (int i = 0; i < 3; ++i) FillArea(px + f(s, 0.42) + i * f(s, 0.08), py + f(s, 0.54), 3, f(s, 0.10), kBlack);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d", hp);
    drawNumber(buf, px, py, s, font, kWhite);
}

void coin(int px, int py, int s, int amount, ifont *font) {
    fillCircle(px + f(s, 0.5), py + f(s, 0.38), f(s, 0.25), kGoldDark);
    fillCircle(px + f(s, 0.5), py + f(s, 0.38), f(s, 0.19), kGoldLight);
    FillArea(px + f(s, 0.5) - 3, py + f(s, 0.38) - f(s, 0.10), 7, f(s, 0.20), kGoldDark);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d", amount);
    drawNumber(buf, px, py, s, font, kBlack);
}

void potion(int px, int py, int s, int heal, ifont *font) {
    FillArea(px + f(s, 0.44), py + f(s, 0.13), f(s, 0.12), f(s, 0.07), kCork);       // cork
    FillArea(px + f(s, 0.45), py + f(s, 0.20), f(s, 0.10), f(s, 0.14), kWhite);      // neck
    fillCircle(px + f(s, 0.5), py + f(s, 0.46), f(s, 0.19), kWhite);                 // flask
    fillCircle(px + f(s, 0.5), py + f(s, 0.49), f(s, 0.13), kPotionGreen);           // liquid
    char buf[16];
    std::snprintf(buf, sizeof buf, "+%d", heal);
    drawNumber(buf, px, py, s, font, kWhite);
}

void stairs(int px, int py, int s) {
    // Four descending steps.
    for (int i = 0; i < 4; ++i)
        FillArea(px + f(s, 0.18) + i * f(s, 0.14), py + f(s, 0.20) + i * f(s, 0.17), f(s, 0.64) - i * f(s, 0.14), f(s, 0.14), kWhite);
}

}  // namespace

void drawTileIcon(const inkdelve::Tile &t, int px, int py, int s, ifont *font) {
    using inkdelve::TileKind;
    switch (t.kind) {
    case TileKind::Monster: monster(px, py, s, t.value, font); break;
    case TileKind::Gold:    coin(px, py, s, t.value, font); break;
    case TileKind::Potion:  potion(px, py, s, t.value, font); break;
    case TileKind::Stairs:  stairs(px, py, s); break;
    default: break;
    }
}

void drawX(int px, int py, int s) {
    const int a = f(s, 0.28), b = f(s, 0.72);
    for (int o = -3; o <= 3; ++o) {
        DrawLine(px + a + o, py + a, px + b + o, py + b, kWhite);
        DrawLine(px + b + o, py + a, px + a + o, py + b, kWhite);
    }
}

void drawHeart(int cx, int cy) {
    const int u = 24;
    fillCircle(cx - u / 2 - 2, cy - u / 3, u * 11 / 20 + 4, kRed);
    fillCircle(cx + u / 2 + 2, cy - u / 3, u * 11 / 20 + 4, kRed);
    fillTriangle(cx - u - 4, cy - u / 8, cx + u + 4, cy - u / 8, cx, cy + u + 4, kRed);
}

void drawCoin(int cx, int cy) {
    fillCircle(cx, cy, 28, kGoldDark);
    fillCircle(cx, cy, 21, kGoldLight);
    FillArea(cx - 3, cy - 11, 7, 22, kGoldDark);
}

void drawDepth(int cx, int cy) {
    for (int i = 0; i < 3; ++i)
        FillArea(cx - 28 + i * 14, cy - 24 + i * 16, 56 - i * 14, 12, kBlue);
}

}  // namespace gfx
