// Portable game core: no UI, no platform headers.
#pragma once
#include <cstdint>
#include <vector>

namespace inkdelve {

enum class TileKind : uint8_t { Empty, Monster, Gold, Potion, Stairs };

struct Tile {
    TileKind kind = TileKind::Empty;
    bool revealed = false;
    int value = 0;  // Monster: current HP. Gold: amount. Potion: heal amount.
};

class Grid {
public:
    static const int kMinStairsDistance = 4;  // Manhattan distance from the entrance

    // Builds a deterministic level from `seed`. One edge tile is revealed as the entrance.
    Grid(int width, int height, uint32_t seed, int depth = 1);

    int width() const { return w_; }
    int height() const { return h_; }
    const Tile &at(int x, int y) const { return tiles_[y * w_ + x]; }
    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }

    // True if a revealed (living) monster is adjacent to (x,y), diagonals included:
    // the tile is locked until the monster is killed.
    bool blockedByMonster(int x, int y) const;
    // A tile can be revealed if it is hidden, 4-adjacent to a revealed tile, and
    // not locked by a neighboring revealed monster.
    bool canReveal(int x, int y) const;
    // Reveals the tile if allowed; returns whether it did.
    bool reveal(int x, int y);

    int revealedCount() const;

    // Used by Game (and tests) to change a tile after generation.
    void setTile(int x, int y, TileKind kind, int value, bool revealed);

private:
    int w_, h_;
    std::vector<Tile> tiles_;
};

}  // namespace inkdelve
