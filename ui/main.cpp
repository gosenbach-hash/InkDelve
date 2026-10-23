#include <cstdio>
#include <ctime>
#include <fstream>
#include <memory>
#include <sstream>

#include "inkview.h"
#include "game.h"
#include "gfx.h"

using namespace inkdelve;

static const int kCols = Game::kCols;
static const int kRows = Game::kRows;
static const int kMargin = 20;
static const int kGap = 8;
static const int kHeader = 120;
static const int kMsgH = 90;
static const int kBtnH = 100;

static ifont *font = nullptr;
static std::unique_ptr<Game> game;
static char message[96] = "Tap a tile next to the revealed one";
static int cell = 0, gx = 0, gy = 0, gw = 0, gh = 0;  // layout, computed on show
static int msgY = 0, btnY = 0, scoreY = 0;
static const int kScoreH = 90;

static const char *kSavePath = STATEPATH "/inkdelve.sav";
static const char *kSaveTmp = STATEPATH "/inkdelve.sav.tmp";

// Saves after every move; a finished (dead) run deletes the save instead.
static void saveGame() {
    if (!game) return;
    if (game->over()) {
        std::remove(kSavePath);
        return;
    }
    {
        std::ofstream f(kSaveTmp, std::ios::binary | std::ios::trunc);
        if (!f) return;
        f << game->serialize();
        if (!f) return;
    }
    std::rename(kSaveTmp, kSavePath);  // write-then-rename so a crash never leaves a half save
}

static bool loadGame() {
    std::ifstream f(kSavePath, std::ios::binary);
    if (!f) return false;
    std::stringstream ss;
    ss << f.rdbuf();
    return Game::load(ss.str(), *game);
}

static const char *kHiPath = STATEPATH "/inkdelve.hi";
static HighScore best;

static void loadHigh() {
    std::ifstream f(kHiPath, std::ios::binary);
    if (!f) return;
    std::stringstream ss;
    ss << f.rdbuf();
    HighScore::load(ss.str(), best);
}

static void saveHigh() {
    std::ofstream f(kHiPath, std::ios::binary | std::ios::trunc);
    if (f) f << best.serialize();
}

// Records the current run against the high score; returns true if it is a new best.
static bool recordRun() {
    if (!game || !best.update(game->score(), game->depth())) return false;
    saveHigh();
    return true;
}

static void newGame() {
    game.reset(new Game(static_cast<uint32_t>(std::time(nullptr))));
    std::snprintf(message, sizeof message, "Tap a tile next to the revealed one");
}

static void layout() {
    const int sw = ScreenWidth(), sh = ScreenHeight();
    const int availW = sw - 2 * kMargin;
    const int availH = sh - kHeader - 4 * kMargin - kMsgH - kBtnH;
    const int cw = (availW - (kCols - 1) * kGap) / kCols;
    const int ch = (availH - (kRows - 1) * kGap) / kRows;
    cell = cw < ch ? cw : ch;
    gw = kCols * cell + (kCols - 1) * kGap;
    gh = kRows * cell + (kRows - 1) * kGap;
    gx = (sw - gw) / 2;
    gy = kHeader + kMargin;
    msgY = gy + gh + kMargin;
    btnY = msgY + kMsgH + kMargin;
    scoreY = btnY + kBtnH + kMargin;
}

static int tileColor(const Tile &t) {
    if (!t.revealed) return 0x00555555;
    switch (t.kind) {
    case TileKind::Monster: return 0x00CC0000;
    case TileKind::Gold:    return 0x00FFCC00;
    case TileKind::Potion:  return 0x0000AA00;
    case TileKind::Stairs:  return 0x000000CC;
    default:                return 0x00DDDDDD;
    }
}

static void drawHud() {
    char buf[32];
    const Hero &h = game->hero();
    const int third = ScreenWidth() / 3;
    const int cy = kHeader / 2 + 10;
    FillArea(0, 0, ScreenWidth(), kHeader, 0x00FFFFFF);
    SetFont(font, 0x00000000);

    gfx::drawHeart(40 + 30, cy);
    std::snprintf(buf, sizeof buf, "%d/%d", h.hp, h.maxHp);
    DrawTextRect(40 + 80, 20, third - 120, kHeader - 20, buf, ALIGN_LEFT | VALIGN_MIDDLE);

    gfx::drawCoin(third + 40 + 30, cy);
    std::snprintf(buf, sizeof buf, "%d", h.gold);
    DrawTextRect(third + 40 + 80, 20, third - 120, kHeader - 20, buf, ALIGN_LEFT | VALIGN_MIDDLE);

    gfx::drawDepth(2 * third + 40 + 30, cy);
    std::snprintf(buf, sizeof buf, "%d", game->depth());
    DrawTextRect(2 * third + 40 + 80, 20, third - 120, kHeader - 20, buf, ALIGN_LEFT | VALIGN_MIDDLE);
}

static void drawGrid() {
    const Grid &grid = game->grid();
    for (int y = 0; y < kRows; ++y) {
        for (int x = 0; x < kCols; ++x) {
            const Tile &t = grid.at(x, y);
            const int px = gx + x * (cell + kGap);
            const int py = gy + y * (cell + kGap);
            FillArea(px, py, cell, cell, tileColor(t));
            // Frontier tiles (tappable) get a thick black border.
            if (!game->over() && grid.canReveal(x, y)) {
                DrawRect(px, py, cell, cell, 0x00000000);
                DrawRect(px + 1, py + 1, cell - 2, cell - 2, 0x00000000);
                DrawRect(px + 2, py + 2, cell - 4, cell - 4, 0x00000000);
            }
            if (!t.revealed) {
                if (grid.blockedByMonster(x, y)) {
                    gfx::drawX(px, py, cell);  // locked by an adjacent monster
                } else {
                    SetFont(font, 0x00FFFFFF);
                    DrawTextRect(px, py, cell, cell, "?", ALIGN_CENTER | VALIGN_MIDDLE);
                }
            } else {
                gfx::drawTileIcon(t, px, py, cell, font);
            }
        }
    }
}

static void drawMessage() {
    FillArea(0, msgY, ScreenWidth(), kMsgH, 0x00FFFFFF);
    SetFont(font, 0x00000000);
    DrawTextRect(kMargin, msgY, ScreenWidth() - 2 * kMargin, kMsgH, message, ALIGN_CENTER | VALIGN_MIDDLE);
}

static void drawButton() {
    DrawRect(gx, btnY, gw, kBtnH, 0x00000000);
    SetFont(font, 0x00000000);
    DrawTextRect(gx, btnY, gw, kBtnH, "New game", ALIGN_CENTER | VALIGN_MIDDLE);
}

// Shop screen: three stacked buttons in the board area.
static const int kShopTitleH = 150;
static const int kShopBtnGap = 30;

static int shopBtnH() { return (gh - kShopTitleH - 2 * kShopBtnGap) / 3; }
static int shopBtnY(int i) { return gy + kShopTitleH + i * (shopBtnH() + kShopBtnGap); }

static void drawShop() {
    const Hero &h = game->hero();
    char buf[64];
    FillArea(gx, gy, gw, gh, 0x00FFFFFF);
    SetFont(font, 0x00000000);
    DrawTextRect(gx, gy, gw, kShopTitleH / 2, "Shop", ALIGN_CENTER | VALIGN_MIDDLE);
    std::snprintf(buf, sizeof buf, "Attack %d    Max HP %d", h.attack, h.maxHp);
    DrawTextRect(gx, gy + kShopTitleH / 2, gw, kShopTitleH / 2, buf, ALIGN_CENTER | VALIGN_MIDDLE);

    const bool canBuy = h.gold >= Game::kShopPrice;
    const char *labels[3] = {"+1 Attack  (100 gold)", "Heal + 5 max HP  (100 gold)", "Continue"};
    for (int i = 0; i < 3; ++i) {
        const int y = shopBtnY(i), bh = shopBtnH();
        const bool enabled = (i == 2) || canBuy;
        const int color = enabled ? 0x00000000 : 0x00999999;
        DrawRect(gx, y, gw, bh, color);
        DrawRect(gx + 1, y + 1, gw - 2, bh - 2, color);
        DrawRect(gx + 2, y + 2, gw - 4, bh - 4, color);
        SetFont(font, color);
        DrawTextRect(gx, y, gw, bh, labels[i], ALIGN_CENTER | VALIGN_MIDDLE);
    }
}

static void drawBoard() {
    if (game->shopOpen()) drawShop();
    else drawGrid();
}

static void drawScore() {
    char buf[96];
    if (best.score > 0)
        std::snprintf(buf, sizeof buf, "Score %d    Best %d (floor %d)", game->score(), best.score, best.floor);
    else
        std::snprintf(buf, sizeof buf, "Score %d    Best -", game->score());
    FillArea(0, scoreY, ScreenWidth(), kScoreH, 0x00FFFFFF);
    SetFont(font, 0x00000000);
    DrawTextRect(kMargin, scoreY, ScreenWidth() - 2 * kMargin, kScoreH, buf, ALIGN_CENTER | VALIGN_MIDDLE);
}

static void drawAll() {
    ClearScreen();
    drawHud();
    drawBoard();
    drawMessage();
    drawButton();
    drawScore();
}

static void setMessage(TapResult r, const Tile &before) {
    switch (r) {
    case TapResult::Revealed:
        if (before.kind == TileKind::Monster) std::snprintf(message, sizeof message, "Monster! Tap it to fight");
        else if (before.kind == TileKind::Stairs) std::snprintf(message, sizeof message, "Stairs! Tap to descend");
        else if (before.kind == TileKind::Gold) std::snprintf(message, sizeof message, "Gold! Tap to collect");
        else if (before.kind == TileKind::Potion) std::snprintf(message, sizeof message, "Potion! Tap to drink");
        else std::snprintf(message, sizeof message, "Nothing here");
        break;
    case TapResult::Collected:
        if (before.kind == TileKind::Gold) std::snprintf(message, sizeof message, "Found %d gold", before.value);
        else std::snprintf(message, sizeof message, "Potion: healed up to %d", before.value);
        break;
    case TapResult::Attacked:
        std::snprintf(message, sizeof message, "It hit you for %d, you hit back", game->monsterDamage());
        break;
    case TapResult::Killed:
        std::snprintf(message, sizeof message, "It hit you for %d. Monster slain", game->monsterDamage());
        break;
    case TapResult::Cleared:
        std::snprintf(message, sizeof message, "Board cleared! +%d gold", game->lastBonus());
        break;
    case TapResult::Descended:
        if (game->shopOpen()) std::snprintf(message, sizeof message, "Depth %d cleared! Spend your gold", game->depth() - 1);
        else std::snprintf(message, sizeof message, "Depth %d", game->depth());
        break;
    case TapResult::HeroDied:
        if (recordRun()) std::snprintf(message, sizeof message, "You died on floor %d. New high score!", game->depth());
        else std::snprintf(message, sizeof message, "You died on floor %d", game->depth());
        break;
    default:
        break;
    }
}

static int handler(int type, int par1, int par2) {
    switch (type) {
    case EVT_INIT:
        SetOrientation(0);             // lock portrait
        SetPanelType(PANEL_DISABLED);  // no system status bar overlapping the layout
        font = OpenFont("LiberationSans", 48, 1);
        loadHigh();
        newGame();
        if (loadGame()) std::snprintf(message, sizeof message, "Run resumed");
        break;
    case EVT_ORIENTATION:
    case EVT_SHOW:
        layout();
        drawAll();
        FullUpdate();
        break;
    case EVT_POINTERUP: {
        const int px = par1, py = par2;
        // New game button: full redraw and full refresh to clear ghosting.
        if (px >= gx && px < gx + gw && py >= btnY && py < btnY + kBtnH) {
            if (!game->over()) recordRun();  // an abandoned run still counts (a death already did)
            newGame();
            saveGame();
            drawAll();
            FullUpdate();
            break;
        }
        // Shop taps: buy upgrades or continue to the next level.
        if (game->shopOpen()) {
            if (px < gx || px >= gx + gw) break;
            int hit = -1;
            for (int i = 0; i < 3; ++i)
                if (py >= shopBtnY(i) && py < shopBtnY(i) + shopBtnH()) hit = i;
            if (hit < 0) break;
            if (hit == 2) {
                game->leaveShop();
                std::snprintf(message, sizeof message, "Depth %d", game->depth());
                saveGame();
                drawAll();
                FullUpdate();  // back to the board: clear ghosting
                break;
            }
            const bool ok = (hit == 0) ? game->buyDamage() : game->buyHeal();
            if (!ok) std::snprintf(message, sizeof message, "Not enough gold");
            else if (hit == 0) std::snprintf(message, sizeof message, "Attack is now %d", game->hero().attack);
            else std::snprintf(message, sizeof message, "Max HP %d, fully healed", game->hero().maxHp);
            saveGame();
            drawHud();
            drawShop();
            drawMessage();
            drawScore();
            PartialUpdate(0, 0, ScreenWidth(), scoreY + kScoreH);
            break;
        }
        // Grid tap: map to cell, ignore taps in the gaps.
        if (px >= gx && px < gx + gw && py >= gy && py < gy + gh) {
            const int rx = (px - gx) % (cell + kGap);
            const int ry = (py - gy) % (cell + kGap);
            if (rx < cell && ry < cell) {
                const int cx = (px - gx) / (cell + kGap);
                const int cy = (py - gy) / (cell + kGap);
                const Tile before = game->grid().at(cx, cy);
                const TapResult r = game->tap(cx, cy);
                if (r == TapResult::Nothing) break;
                setMessage(r, before);
                saveGame();
                if (r == TapResult::Descended) {
                    drawAll();
                    FullUpdate();  // new level: clear ghosting
                } else {
                    drawHud();
                    drawBoard();
                    drawMessage();
                    drawScore();
                    PartialUpdate(0, 0, ScreenWidth(), scoreY + kScoreH);
                }
            }
        }
        break;
    }
    case EVT_KEYPRESS:
        if (par1 == IV_KEY_PREV || par1 == IV_KEY_HOME || par1 == IV_KEY_BACK) CloseApp();
        break;
    case EVT_EXIT:
        saveGame();
        if (font) CloseFont(font);
        break;
    }
    return 0;
}

int main(int, char **) {
    InkViewMain(handler);
    return 0;
}
