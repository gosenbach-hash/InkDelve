#include "grid.h"

namespace inkdelve {

namespace {
// Small deterministic PRNG (xorshift32); same output on every platform.
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 0x9E3779B9u) {}
    uint32_t next() {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    int below(int n) { return static_cast<int>(next() % static_cast<uint32_t>(n)); }
};
}  // namespace

Grid::Grid(int width, int height, uint32_t seed, int depth) : w_(width), h_(height), tiles_(width * height) {
    Rng rng(seed);
    const int n = w_ * h_;

    // Entrance: random tile on the outer edge, revealed, always empty.
    std::vector<int> edge;
    for (int i = 0; i < n; ++i) {
        const int x = i % w_, y = i / w_;
        if (x == 0 || y == 0 || x == w_ - 1 || y == h_ - 1) edge.push_back(i);
    }
    const int entrance = edge[rng.below(static_cast<int>(edge.size()))];
    tiles_[entrance].revealed = true;

    // Stairs: random hidden tile at least kMinStairsDistance (Manhattan) from the entrance.
    std::vector<int> far;
    for (int i = 0; i < n; ++i) {
        const int dx = (i % w_) - (entrance % w_), dy = (i / w_) - (entrance / w_);
        if ((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy) >= kMinStairsDistance) far.push_back(i);
    }
    int stairs;
    if (!far.empty()) {
        stairs = far[rng.below(static_cast<int>(far.size()))];
    } else {  // tiny grid: fall back to any other tile
        do { stairs = rng.below(n); } while (stairs == entrance);
    }
    tiles_[stairs].kind = TileKind::Stairs;

    // Fill the rest: 40% monsters, 20% gold, 12% potions, 28% empty.
    for (int i = 0; i < n; ++i) {
        if (i == entrance || i == stairs) continue;
        int r = rng.below(100);
        // Monsters never touch each other (orthogonally); tiles are filled in row-major
        // order, so only the left and upper neighbors can already be monsters.
        const bool leftMonster = (i % w_) > 0 && tiles_[i - 1].kind == TileKind::Monster;
        const bool upMonster = i >= w_ && tiles_[i - w_].kind == TileKind::Monster;
        if (r < 40 && (leftMonster || upMonster)) r = 40 + rng.below(60);
        if (r < 40) {
            tiles_[i].kind = TileKind::Monster;
            tiles_[i].value = 2 + depth + rng.below(3);
        } else if (r < 60) {
            tiles_[i].kind = TileKind::Gold;
            tiles_[i].value = 1 + depth + rng.below(5);
        } else if (r < 72) {
            tiles_[i].kind = TileKind::Potion;
            tiles_[i].value = 5;
        }
    }
}

namespace {
const int kDx[4] = {1, -1, 0, 0};
const int kDy[4] = {0, 0, 1, -1};
}  // namespace

bool Grid::blockedByMonster(int x, int y) const {
    // All 8 neighbors count, diagonals included.
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const int nx = x + dx, ny = y + dy;
            if (inBounds(nx, ny) && at(nx, ny).revealed && at(nx, ny).kind == TileKind::Monster) return true;
        }
    }
    return false;
}

bool Grid::canReveal(int x, int y) const {
    if (!inBounds(x, y) || at(x, y).revealed) return false;
    // A living monster locks every tile next to it until it is killed.
    if (blockedByMonster(x, y)) return false;
    for (int i = 0; i < 4; ++i) {
        const int nx = x + kDx[i], ny = y + kDy[i];
        if (inBounds(nx, ny) && at(nx, ny).revealed) return true;
    }
    return false;
}

bool Grid::reveal(int x, int y) {
    if (!canReveal(x, y)) return false;
    tiles_[y * w_ + x].revealed = true;
    return true;
}

int Grid::revealedCount() const {
    int c = 0;
    for (const Tile &t : tiles_) c += t.revealed ? 1 : 0;
    return c;
}

void Grid::setTile(int x, int y, TileKind kind, int value, bool revealed) {
    Tile &t = tiles_[y * w_ + x];
    t.kind = kind;
    t.value = value;
    t.revealed = revealed;
}

}  // namespace inkdelve
