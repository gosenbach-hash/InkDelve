#include "game.h"

#include <sstream>

namespace inkdelve {

Game::Game(uint32_t seed) : seed_(seed), depth_(1), grid_(kCols, kRows, seed, 1) {}

// Every tile revealed and nothing left but empty ground and the stairs.
bool Game::boardCleared() const {
    for (int y = 0; y < grid_.height(); ++y) {
        for (int x = 0; x < grid_.width(); ++x) {
            const Tile &t = grid_.at(x, y);
            if (!t.revealed || (t.kind != TileKind::Empty && t.kind != TileKind::Stairs)) return false;
        }
    }
    return true;
}

// Called after an action that changed the board; awards the clear bonus once, when it happens.
TapResult Game::afterBoardChange(TapResult r) {
    lastBonus_ = 0;
    if (!boardCleared()) return r;
    lastBonus_ = 10 + 5 * depth_;
    hero_.gold += lastBonus_;
    totalGold_ += lastBonus_;
    return TapResult::Cleared;
}

TapResult Game::tap(int x, int y) {
    if (over() || shopOpen_ || !grid_.inBounds(x, y)) return TapResult::Nothing;
    const Tile t = grid_.at(x, y);

    if (!t.revealed) {
        return grid_.reveal(x, y) ? afterBoardChange(TapResult::Revealed) : TapResult::Nothing;
    }

    if (t.kind == TileKind::Gold) {
        hero_.gold += t.value;
        totalGold_ += t.value;
        grid_.setTile(x, y, TileKind::Empty, 0, true);
        return afterBoardChange(TapResult::Collected);
    }

    if (t.kind == TileKind::Potion) {
        hero_.hp += t.value;
        if (hero_.hp > hero_.maxHp) hero_.hp = hero_.maxHp;
        grid_.setTile(x, y, TileKind::Empty, 0, true);
        return afterBoardChange(TapResult::Collected);
    }

    if (t.kind == TileKind::Monster) {
        // The monster always strikes first; the hero only hits back if still alive.
        hero_.hp -= monsterDamage();
        if (hero_.hp <= 0) {
            hero_.hp = 0;
            return TapResult::HeroDied;
        }
        const int left = t.value - hero_.attack;
        if (left <= 0) {
            grid_.setTile(x, y, TileKind::Empty, 0, true);
            ++kills_;
            return afterBoardChange(TapResult::Killed);
        }
        grid_.setTile(x, y, TileKind::Monster, left, true);
        return TapResult::Attacked;
    }

    if (t.kind == TileKind::Stairs) {
        // Finishing every kShopEvery-th level opens the shop before the next one.
        shopOpen_ = (depth_ % kShopEvery) == 0;
        ++depth_;
        grid_ = Grid(kCols, kRows, seed_ + static_cast<uint32_t>(depth_) * 7919u, depth_);
        return TapResult::Descended;
    }

    return TapResult::Nothing;
}

bool Game::buyDamage() {
    if (!shopOpen_ || hero_.gold < kShopPrice) return false;
    hero_.gold -= kShopPrice;
    hero_.attack += 1;
    return true;
}

bool Game::buyHeal() {
    if (!shopOpen_ || hero_.gold < kShopPrice) return false;
    hero_.gold -= kShopPrice;
    hero_.maxHp += 5;
    hero_.hp = hero_.maxHp;
    return true;
}

void Game::leaveShop() { shopOpen_ = false; }

// Format: "INKDELVE3 seed depth hp maxHp attack gold kills totalGold shop w h" then w*h triples
// "kind value revealed". Older saves still load: INKDELVE2 has no kills/totalGold, INKDELVE1
// also has no shop (missing stats default to 0 kills and totalGold = current gold).
std::string Game::serialize() const {
    std::ostringstream os;
    os << "INKDELVE3 " << seed_ << ' ' << depth_ << ' ' << hero_.hp << ' ' << hero_.maxHp << ' '
       << hero_.attack << ' ' << hero_.gold << ' ' << kills_ << ' ' << totalGold_ << ' '
       << (shopOpen_ ? 1 : 0) << ' ' << grid_.width() << ' ' << grid_.height();
    for (int y = 0; y < grid_.height(); ++y) {
        for (int x = 0; x < grid_.width(); ++x) {
            const Tile &t = grid_.at(x, y);
            os << ' ' << static_cast<int>(t.kind) << ' ' << t.value << ' ' << (t.revealed ? 1 : 0);
        }
    }
    os << '\n';
    return os.str();
}

bool Game::load(const std::string &text, Game &out) {
    std::istringstream is(text);
    std::string magic;
    unsigned long seed;
    int depth, w, h, shop = 0, kills = 0, totalGold = -1;
    Hero hero;
    if (!(is >> magic >> seed >> depth >> hero.hp >> hero.maxHp >> hero.attack >> hero.gold)) return false;
    if (magic == "INKDELVE3") {
        if (!(is >> kills >> totalGold >> shop)) return false;
        if (kills < 0 || totalGold < 0 || (shop != 0 && shop != 1)) return false;
    } else if (magic == "INKDELVE2") {
        if (!(is >> shop) || (shop != 0 && shop != 1)) return false;
    } else if (magic != "INKDELVE1") {
        return false;
    }
    if (totalGold < 0) totalGold = hero.gold;
    if (!(is >> w >> h)) return false;
    if (w != kCols || h != kRows || depth < 1 || depth > 10000) return false;
    if (hero.maxHp < 1 || hero.hp < 1 || hero.hp > hero.maxHp || hero.attack < 1 || hero.gold < 0) return false;

    Game g(static_cast<uint32_t>(seed));
    g.depth_ = depth;
    g.hero_ = hero;
    g.shopOpen_ = shop == 1;
    g.kills_ = kills;
    g.totalGold_ = totalGold;
    int revealed = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int kind, value, rev;
            if (!(is >> kind >> value >> rev)) return false;
            if (kind < 0 || kind > static_cast<int>(TileKind::Stairs) || value < 0 || (rev != 0 && rev != 1)) return false;
            g.grid_.setTile(x, y, static_cast<TileKind>(kind), value, rev == 1);
            revealed += rev;
        }
    }
    if (revealed < 1) return false;
    out = g;
    return true;
}

bool HighScore::update(int newScore, int newFloor) {
    if (newScore <= score) return false;
    score = newScore;
    floor = newFloor;
    return true;
}

// Format: "INKDELVEHI1 score floor".
std::string HighScore::serialize() const {
    std::ostringstream os;
    os << "INKDELVEHI1 " << score << ' ' << floor << '\n';
    return os.str();
}

bool HighScore::load(const std::string &text, HighScore &out) {
    std::istringstream is(text);
    std::string magic;
    int s, f;
    if (!(is >> magic >> s >> f) || magic != "INKDELVEHI1" || s < 0 || f < 0) return false;
    out.score = s;
    out.floor = f;
    return true;
}

}  // namespace inkdelve
