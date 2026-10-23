#include <cstdio>
#include "grid.h"

using namespace inkdelve;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)

int main() {
    // Exactly one tile revealed at start, and it is empty.
    Grid g(5, 4, 12345);
    CHECK(g.revealedCount() == 1);

    // Find the entrance.
    int ex = -1, ey = -1, stairs = 0;
    for (int y = 0; y < g.height(); ++y)
        for (int x = 0; x < g.width(); ++x) {
            if (g.at(x, y).revealed) { ex = x; ey = y; }
            if (g.at(x, y).kind == TileKind::Stairs) ++stairs;
        }
    CHECK(ex >= 0);
    CHECK(g.at(ex, ey).kind == TileKind::Empty);
    CHECK(stairs == 1);

    // Cannot reveal the already-revealed entrance.
    CHECK(!g.canReveal(ex, ey));

    // Out of bounds is rejected.
    CHECK(!g.canReveal(-1, 0));
    CHECK(!g.reveal(g.width(), 0));

    // A far, non-adjacent tile cannot be revealed; an adjacent one can.
    int fx = -1, fy = -1;
    for (int y = 0; y < g.height() && fx < 0; ++y)
        for (int x = 0; x < g.width(); ++x) {
            int d = (x > ex ? x - ex : ex - x) + (y > ey ? y - ey : ey - y);
            if (d >= 2) { fx = x; fy = y; break; }
        }
    CHECK(fx >= 0);
    CHECK(!g.reveal(fx, fy));
    CHECK(g.revealedCount() == 1);

    // Diagonal neighbors do not count as adjacent.
    if (g.inBounds(ex + 1, ey + 1)) CHECK(!g.canReveal(ex + 1, ey + 1));

    // Revealing orthogonal neighbors works and expands the frontier.
    int nx = ex + 1 < g.width() ? ex + 1 : ex - 1;
    CHECK(g.reveal(nx, ey));
    CHECK(g.revealedCount() == 2);
    CHECK(!g.reveal(nx, ey));  // already revealed

    // Everything is eventually reachable by repeated reveals (clearing monsters as we go).
    bool progress = true;
    while (progress) {
        progress = false;
        for (int y = 0; y < g.height(); ++y)
            for (int x = 0; x < g.width(); ++x) {
                if (g.reveal(x, y)) progress = true;
                if (g.at(x, y).revealed && g.at(x, y).kind == TileKind::Monster) {
                    g.setTile(x, y, TileKind::Empty, 0, true);  // monster killed
                    progress = true;
                }
            }
    }
    CHECK(g.revealedCount() == g.width() * g.height());

    // A revealed monster blocks: tiles beyond it stay unreachable until it is killed.
    {
        Grid m(5, 4, 1);
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 5; ++x) m.setTile(x, y, TileKind::Empty, 0, false);
        m.setTile(0, 0, TileKind::Empty, 0, true);
        m.setTile(1, 0, TileKind::Monster, 5, true);
        CHECK(!m.canReveal(2, 0));   // only neighbor is the monster
        // The diagonal tile (0,1) is next to the entrance but also locked by the monster.
        CHECK(m.blockedByMonster(0, 1) && !m.canReveal(0, 1));
        CHECK(m.blockedByMonster(1, 1) && !m.canReveal(1, 1));
        CHECK(!m.blockedByMonster(0, 2));
        m.setTile(1, 0, TileKind::Empty, 0, true);  // monster dies: everything unlocks
        CHECK(!m.blockedByMonster(0, 1) && !m.blockedByMonster(1, 1));
        CHECK(m.canReveal(0, 1));
        CHECK(m.canReveal(2, 0));
    }
    // A diagonal monster locks a tile that has an open orthogonal route.
    {
        Grid m(5, 4, 1);
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 5; ++x) m.setTile(x, y, TileKind::Empty, 0, false);
        m.setTile(0, 0, TileKind::Empty, 0, true);
        m.setTile(1, 0, TileKind::Empty, 0, true);
        m.setTile(2, 2, TileKind::Monster, 5, true);
        CHECK(!m.canReveal(1, 1));  // orthogonal route via (1,0), but the monster is diagonal
        CHECK(m.canReveal(0, 1));   // far from the monster
        CHECK(m.canReveal(2, 0));   // far from the monster
    }

    // Generation never places two monsters next to each other.
    for (uint32_t seed = 1; seed <= 500; ++seed) {
        Grid q(6, 5, seed, 1 + static_cast<int>(seed % 5));
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 6; ++x) {
                if (q.at(x, y).kind != TileKind::Monster) continue;
                if (x + 1 < 6) CHECK(q.at(x + 1, y).kind != TileKind::Monster);
                if (y + 1 < 5) CHECK(q.at(x, y + 1).kind != TileKind::Monster);
            }
    }

    // Entrance is always on the edge; stairs are at least 4 tiles (Manhattan) away; stats sane.
    int golds = 0, potions = 0, tiles = 0;
    for (uint32_t seed = 1; seed <= 500; ++seed) {
        Grid q(6, 5, seed);
        int ex = -1, ey = -1, sx = -1, sy = -1, revealed = 0;
        for (int y = 0; y < 5; ++y)
            for (int x = 0; x < 6; ++x) {
                if (q.at(x, y).revealed) { ex = x; ey = y; ++revealed; }
                if (q.at(x, y).kind == TileKind::Stairs) { sx = x; sy = y; }
                if (q.at(x, y).kind == TileKind::Gold) ++golds;
                if (q.at(x, y).kind == TileKind::Potion) ++potions;
                ++tiles;
            }
        CHECK(revealed == 1);
        CHECK(q.entrance() == ey * 6 + ex);
        CHECK(ex == 0 || ey == 0 || ex == 5 || ey == 4);
        const int d = (sx > ex ? sx - ex : ex - sx) + (sy > ey ? sy - ey : ey - sy);
        CHECK(d >= Grid::kMinStairsDistance);
    }
    // Roughly 20% gold and 12% potions (loose bounds).
    CHECK(golds * 100 / tiles >= 14 && golds * 100 / tiles <= 24);
    CHECK(potions * 100 / tiles >= 8 && potions * 100 / tiles <= 16);

    // Determinism: same seed gives the same layout; different seed differs somewhere.
    Grid a(5, 4, 777), b(5, 4, 777), c(5, 4, 778);
    bool same = true, diff = false;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 5; ++x) {
            if (a.at(x, y).kind != b.at(x, y).kind || a.at(x, y).revealed != b.at(x, y).revealed) same = false;
            if (a.at(x, y).kind != c.at(x, y).kind || a.at(x, y).revealed != c.at(x, y).revealed) diff = true;
        }
    CHECK(same);
    CHECK(diff);

    if (failures == 0) std::printf("All tests passed\n");
    return failures ? 1 : 0;
}
