#include "gfx.h"

#include <cmath>
#include <cstdio>

namespace gfx {

namespace {

const int kWhite = 0x00FFFFFF;
const int kBlack = 0x00000000;
const int kRed = 0x00CC0000;
const int kHornRed = 0x00550000;
const int kGoldDark = 0x00B07800;
const int kGoldLight = 0x00FFE066;
const int kGlow = 0x00FFE000;
const int kPotionGreen = 0x00007700;
const int kPotionShine = 0x0088DD88;
const int kCork = 0x00804A18;
const int kBlue = 0x000000CC;
const int kStepFace = 0x00B0B0B0;
const int kFloor = 0x00DDDDDD;
const int kPebble = 0x00BBBBBB;
const int kSkin = 0x00F2C48D;

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

void bigX(int px, int py, int s, int color) {
    const int a = f(s, 0.28), b = f(s, 0.72);
    for (int o = -3; o <= 3; ++o) {
        DrawLine(px + a + o, py + a, px + b + o, py + b, color);
        DrawLine(px + b + o, py + a, px + a + o, py + b, color);
    }
}

// --- tile icons ---

// Skull with horns from floor 3 and glowing eyes from floor 6.
void monster(int px, int py, int s, int hp, ifont *font, int depth) {
    if (depth >= 3) {
        fillTriangle(px + f(s, 0.31), py + f(s, 0.30), px + f(s, 0.41), py + f(s, 0.20), px + f(s, 0.24), py + f(s, 0.07), kHornRed);
        fillTriangle(px + f(s, 0.69), py + f(s, 0.30), px + f(s, 0.59), py + f(s, 0.20), px + f(s, 0.76), py + f(s, 0.07), kHornRed);
    }
    fillCircle(px + f(s, 0.5), py + f(s, 0.36), f(s, 0.23), kWhite);
    FillArea(px + f(s, 0.35), py + f(s, 0.48), f(s, 0.30), f(s, 0.16), kWhite);
    // Eye sockets, with a yellow glint (bigger glow on deep floors).
    const int eyeR = f(s, 0.07);
    fillCircle(px + f(s, 0.42), py + f(s, 0.34), eyeR, kBlack);
    fillCircle(px + f(s, 0.58), py + f(s, 0.34), eyeR, kBlack);
    const int glowR = depth >= 6 ? f(s, 0.05) : f(s, 0.025);
    fillCircle(px + f(s, 0.42), py + f(s, 0.34), glowR, kGlow);
    fillCircle(px + f(s, 0.58), py + f(s, 0.34), glowR, kGlow);
    fillTriangle(px + f(s, 0.47), py + f(s, 0.46), px + f(s, 0.53), py + f(s, 0.46), px + f(s, 0.50), py + f(s, 0.41), kBlack);
    for (int i = 0; i < 3; ++i) FillArea(px + f(s, 0.41) + i * f(s, 0.09), py + f(s, 0.54), 3, f(s, 0.10), kBlack);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d", hp);
    drawNumber(buf, px, py, s, font, kWhite);
}

void coinAt(int cx, int cy, int r, ifont *font, bool mark) {
    fillCircle(cx, cy, r, kBlack);
    fillCircle(cx, cy, r - 3, kGoldDark);
    fillCircle(cx, cy, r - 8, kGoldLight);
    if (mark) {
        SetFont(font, kGoldDark);
        DrawTextRect(cx - r, cy - r, 2 * r, 2 * r, "$", ALIGN_CENTER | VALIGN_MIDDLE);
    }
}

// One coin, or a pair for bigger piles, with a "$" stamp.
void coin(int px, int py, int s, int amount, ifont *font) {
    const int r = f(s, 0.21);
    if (amount >= 8) coinAt(px + f(s, 0.40), py + f(s, 0.34), r, font, false);
    coinAt(px + f(s, amount >= 8 ? 0.58 : 0.5), py + f(s, amount >= 8 ? 0.42 : 0.38), r, font, true);
    char buf[16];
    std::snprintf(buf, sizeof buf, "%d", amount);
    drawNumber(buf, px, py, s, font, kBlack);
}

void potion(int px, int py, int s, int heal, ifont *font) {
    FillArea(px + f(s, 0.43), py + f(s, 0.12), f(s, 0.14), f(s, 0.07), kCork);             // cork
    FillArea(px + f(s, 0.45), py + f(s, 0.19), f(s, 0.10), f(s, 0.15), kWhite);            // neck
    FillArea(px + f(s, 0.42), py + f(s, 0.30), f(s, 0.16), f(s, 0.03), kStepFace);         // collar
    fillCircle(px + f(s, 0.5), py + f(s, 0.47), f(s, 0.20), kWhite);                        // flask glass
    fillCircle(px + f(s, 0.5), py + f(s, 0.50), f(s, 0.14), kPotionGreen);                  // liquid
    fillCircle(px + f(s, 0.44), py + f(s, 0.46), f(s, 0.035), kPotionShine);                // shine
    FillArea(px + f(s, 0.34), py + f(s, 0.40), 3, f(s, 0.12), kWhite);                      // glass glint
    char buf[16];
    std::snprintf(buf, sizeof buf, "+%d", heal);
    drawNumber(buf, px, py, s, font, kWhite);
}

// Four stone steps, each with a shaded front face.
void stairs(int px, int py, int s) {
    for (int i = 0; i < 4; ++i) {
        const int x = px + f(s, 0.18) + i * f(s, 0.14);
        const int y = py + f(s, 0.20) + i * f(s, 0.17);
        const int w = f(s, 0.64) - i * f(s, 0.14);
        FillArea(x, y, w, f(s, 0.12), kWhite);
        FillArea(x, y + f(s, 0.12), w, f(s, 0.05), kStepFace);
    }
}

// Deterministic speckle so empty floor is not a blank slab.
void pebbles(int px, int py, int s) {
    unsigned h = static_cast<unsigned>(px * 73856093u) ^ static_cast<unsigned>(py * 19349663u);
    for (int i = 0; i < 4; ++i) {
        h = h * 1664525u + 1013904223u;
        const int x = px + f(s, 0.15) + static_cast<int>((h >> 8) % static_cast<unsigned>(f(s, 0.70)));
        h = h * 1664525u + 1013904223u;
        const int y = py + f(s, 0.15) + static_cast<int>((h >> 8) % static_cast<unsigned>(f(s, 0.70)));
        fillCircle(x, y, f(s, 0.03) + static_cast<int>(i % 2), kPebble);
    }
}

// The hero: round head, blue tunic, sword.
void hero(int px, int py, int s) {
    fillCircle(px + f(s, 0.46), py + f(s, 0.34), f(s, 0.11), kSkin);
    FillArea(px + f(s, 0.36), py + f(s, 0.22), f(s, 0.20), f(s, 0.07), 0x00553311);          // hair
    fillTriangle(px + f(s, 0.46), py + f(s, 0.46), px + f(s, 0.30), py + f(s, 0.78), px + f(s, 0.62), py + f(s, 0.78), kBlue);
    FillArea(px + f(s, 0.68), py + f(s, 0.34), f(s, 0.04), f(s, 0.34), kBlack);                // blade
    FillArea(px + f(s, 0.63), py + f(s, 0.60), f(s, 0.14), f(s, 0.04), kBlack);                // hilt
}

int floorColor(inkdelve::TileKind k) {
    using inkdelve::TileKind;
    switch (k) {
    case TileKind::Monster: return kRed;
    case TileKind::Gold:    return 0x00FFCC00;
    case TileKind::Potion:  return 0x0000AA00;
    case TileKind::Stairs:  return kBlue;
    default:                return kFloor;
    }
}

}  // namespace

void outline(int x, int y, int w, int h, int t, int color) {
    FillArea(x, y, w, t, color);
    FillArea(x, y + h - t, w, t, color);
    FillArea(x, y, t, h, color);
    FillArea(x + w - t, y, t, h, color);
}

void drawTile(const inkdelve::Tile &t, int px, int py, int s, ifont *font, int depth, bool entrance,
              bool tappable, bool locked) {
    using inkdelve::TileKind;
    if (!t.revealed) {
        // Raised stone button: lit top-left edge, shaded bottom-right edge.
        const int base = locked ? 0x00404040 : (tappable ? 0x00909090 : 0x00505050);
        const int hi = locked ? 0x00606060 : 0x00C0C0C0;
        const int lo = 0x00282828;
        const int b = f(s, 0.04);
        FillArea(px, py, s, s, base);
        FillArea(px, py, s, b, hi);
        FillArea(px, py, b, s, hi);
        FillArea(px, py + s - b, s, b, lo);
        FillArea(px + s - b, py, b, s, lo);
        if (locked) {
            bigX(px, py, s, kWhite);
        } else {
            SetFont(font, tappable ? kWhite : 0x00BBBBBB);
            DrawTextRect(px, py, s, s, "?", ALIGN_CENTER | VALIGN_MIDDLE);
        }
        outline(px, py, s, s, 3, kBlack);
        if (tappable) outline(px + 3, py + 3, s - 6, s - 6, 3, kBlack);
        return;
    }

    FillArea(px, py, s, s, floorColor(t.kind));
    switch (t.kind) {
    case TileKind::Monster: monster(px, py, s, t.value, font, depth); break;
    case TileKind::Gold:    coin(px, py, s, t.value, font); break;
    case TileKind::Potion:  potion(px, py, s, t.value, font); break;
    case TileKind::Stairs:  stairs(px, py, s); break;
    default:
        pebbles(px, py, s);
        if (entrance) hero(px, py, s);
        break;
    }
    outline(px, py, s, s, 3, kBlack);
}

void drawHeart(int cx, int cy) {
    const int u = 24;
    fillCircle(cx - u / 2 - 2, cy - u / 3, u * 11 / 20 + 4, kRed);
    fillCircle(cx + u / 2 + 2, cy - u / 3, u * 11 / 20 + 4, kRed);
    fillTriangle(cx - u - 4, cy - u / 8, cx + u + 4, cy - u / 8, cx, cy + u + 4, kRed);
}

void drawCoin(int cx, int cy) {
    fillCircle(cx, cy, 28, kBlack);
    fillCircle(cx, cy, 25, kGoldDark);
    fillCircle(cx, cy, 20, kGoldLight);
    FillArea(cx - 3, cy - 11, 7, 22, kGoldDark);
}

void drawDepth(int cx, int cy) {
    for (int i = 0; i < 3; ++i)
        FillArea(cx - 28 + i * 14, cy - 24 + i * 16, 56 - i * 14, 12, kBlue);
}

void drawSword(int cx, int cy) {
    fillTriangle(cx - 6, cy - 28, cx + 6, cy - 28, cx, cy - 36, kBlack);  // tip
    FillArea(cx - 6, cy - 28, 12, 44, kBlack);                              // blade
    FillArea(cx - 2, cy - 28, 4, 40, kStepFace);                            // fuller
    FillArea(cx - 20, cy + 14, 40, 8, kBlack);                              // crossguard
    FillArea(cx - 5, cy + 22, 10, 14, kCork);                               // grip
}

void drawHpBar(int x, int y, int w, int h, int hp, int maxHp) {
    FillArea(x, y, w, h, 0x00DDDDDD);
    const int filled = maxHp > 0 ? (w - 6) * hp / maxHp : 0;
    if (filled > 0) FillArea(x + 3, y + 3, filled, h - 6, kRed);
    outline(x, y, w, h, 3, kBlack);
}

}  // namespace gfx
