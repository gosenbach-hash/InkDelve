// Game rules on top of Grid: hero stats, combat, pickups, descending.
#pragma once
#include <cstdint>
#include <string>
#include "grid.h"

namespace inkdelve {

struct Hero {
    int hp = 20;
    int maxHp = 20;
    int attack = 3;
    int gold = 0;
};

enum class TapResult : uint8_t {
    Nothing,    // invalid tap, game over, or an empty revealed tile
    Revealed,   // hidden tile revealed (nothing is collected or fought yet)
    Attacked,   // monster hit the hero, hero hit it, it survived
    Killed,     // monster hit the hero, then died
    Collected,  // gold or potion picked up on reveal
    Descended,  // took the stairs, new level
    HeroDied,   // hero HP reached 0; game is over
    Cleared,    // that action cleared the whole board; bonus gold awarded (see lastBonus)
};

class Game {
public:
    explicit Game(uint32_t seed);

    const Grid &grid() const { return grid_; }
    const Hero &hero() const { return hero_; }
    int depth() const { return depth_; }
    bool over() const { return hero_.hp <= 0; }

    // Scoring: floor reached, monsters slain, and total gold ever acquired
    // (spending in the shop does not lower it).
    static const int kScorePerFloor = 100;
    static const int kScorePerKill = 10;
    int kills() const { return kills_; }
    int totalGold() const { return totalGold_; }
    int score() const { return depth_ * kScorePerFloor + kills_ * kScorePerKill + totalGold_; }

    // The one player action: tap a tile.
    //  hidden + adjacent  -> reveal only
    //  revealed gold/potion -> collect it
    //  revealed monster   -> trade blows (the monster hits first, even on the killing round)
    //  revealed stairs    -> descend
    TapResult tap(int x, int y);

    // True when every tile is revealed and only empty ground and the stairs remain.
    bool boardCleared() const;
    // Gold bonus awarded by the most recent tap (0 unless it returned Cleared).
    int lastBonus() const { return lastBonus_; }

    // Shop: opens after finishing every kShopEvery-th level; taps on the board are ignored
    // while it is open. Each purchase costs kShopPrice gold and can be repeated.
    static const int kShopEvery = 5;
    static const int kShopPrice = 100;
    bool shopOpen() const { return shopOpen_; }
    bool buyDamage();   // +1 attack
    bool buyHeal();     // +5 max HP and heal to full
    void leaveShop();

    // Damage a monster of the current depth deals per hit.
    int monsterDamage() const { return 1 + depth_ / 2; }

    // Save/load as a small text blob (versioned). load() returns false and leaves
    // `out` untouched if the text is malformed or out of range.
    std::string serialize() const;
    static bool load(const std::string &text, Game &out);

    // For tests: direct access to adjust the level and hero.
    Grid &mutableGrid() { return grid_; }
    Hero &mutableHero() { return hero_; }

    static const int kCols = 6;
    static const int kRows = 5;

private:
    TapResult afterBoardChange(TapResult r);

    uint32_t seed_;
    int depth_;
    int lastBonus_ = 0;
    bool shopOpen_ = false;
    int kills_ = 0;
    int totalGold_ = 0;
    Hero hero_;
    Grid grid_;
};

// Best run so far: the score and the floor reached in that run.
struct HighScore {
    int score = 0;
    int floor = 0;

    // Replaces the record if `newScore` beats it; returns whether it did.
    bool update(int newScore, int newFloor);
    std::string serialize() const;
    // Returns false and leaves `out` untouched on malformed input.
    static bool load(const std::string &text, HighScore &out);
};

}  // namespace inkdelve
