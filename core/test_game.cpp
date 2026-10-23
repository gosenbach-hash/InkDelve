#include <cstdio>
#include <sstream>
#include <string>
#include "game.h"

using namespace inkdelve;

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL line %d: %s\n", __LINE__, #cond); ++failures; } } while (0)

// Clears the level to Empty with only the entrance (0,0) revealed.
static void blank(Game &g) {
    Grid &gr = g.mutableGrid();
    for (int y = 0; y < gr.height(); ++y)
        for (int x = 0; x < gr.width(); ++x) gr.setTile(x, y, TileKind::Empty, 0, false);
    gr.setTile(0, 0, TileKind::Empty, 0, true);
}

int main() {
    // Gold: first tap reveals, second tap collects, tile becomes empty.
    {
        Game g(1); blank(g);
        g.mutableGrid().setTile(1, 0, TileKind::Gold, 7, false);
        CHECK(g.tap(1, 0) == TapResult::Revealed);
        CHECK(g.hero().gold == 0);
        CHECK(g.grid().at(1, 0).kind == TileKind::Gold);
        CHECK(g.tap(1, 0) == TapResult::Collected);
        CHECK(g.hero().gold == 7);
        CHECK(g.grid().at(1, 0).kind == TileKind::Empty && g.grid().at(1, 0).revealed);
    }
    // Potion: heals, capped at max HP.
    {
        Game g(1); blank(g);
        g.mutableHero().hp = 10;
        g.mutableGrid().setTile(1, 0, TileKind::Potion, 5, false);
        g.mutableGrid().setTile(2, 0, TileKind::Potion, 5, false);
        CHECK(g.tap(1, 0) == TapResult::Revealed);
        CHECK(g.hero().hp == 10);
        CHECK(g.tap(1, 0) == TapResult::Collected);
        CHECK(g.hero().hp == 15);
        g.mutableHero().hp = 18;
        CHECK(g.tap(2, 0) == TapResult::Revealed);
        CHECK(g.tap(2, 0) == TapResult::Collected);
        CHECK(g.hero().hp == 20);
    }
    // Non-adjacent hidden tile cannot be tapped.
    {
        Game g(1); blank(g);
        g.mutableGrid().setTile(3, 3, TileKind::Gold, 5, false);
        CHECK(g.tap(3, 3) == TapResult::Nothing);
        CHECK(g.hero().gold == 0);
    }
    // Monster: reveal does not fight; taps trade blows; kill leaves an empty tile.
    {
        Game g(1); blank(g);
        g.mutableGrid().setTile(1, 0, TileKind::Monster, 7, false);  // attack 3: 7 -> 4 -> 1 -> dead
        CHECK(g.tap(1, 0) == TapResult::Revealed);
        CHECK(g.hero().hp == 20);
        CHECK(g.tap(1, 0) == TapResult::Attacked);
        CHECK(g.grid().at(1, 0).value == 4);
        CHECK(g.hero().hp == 20 - g.monsterDamage());
        CHECK(g.tap(1, 0) == TapResult::Attacked);
        CHECK(g.tap(1, 0) == TapResult::Killed);  // monster still hits first on the killing round
        CHECK(g.hero().hp == 20 - 3 * g.monsterDamage());
        CHECK(g.grid().at(1, 0).kind == TileKind::Empty);
    }
    // Death ends the game; further taps do nothing.
    {
        Game g(1); blank(g);
        g.mutableHero().hp = 1;
        g.mutableGrid().setTile(1, 0, TileKind::Monster, 50, true);
        CHECK(g.tap(1, 0) == TapResult::HeroDied);
        CHECK(g.over() && g.hero().hp == 0);
        CHECK(g.tap(1, 0) == TapResult::Nothing);
    }
    // Stairs: reveal, then tap again to descend; hero stats carry over; fresh level.
    {
        Game g(1); blank(g);
        g.mutableHero().gold = 9;
        g.mutableGrid().setTile(1, 0, TileKind::Stairs, 0, false);
        CHECK(g.tap(1, 0) == TapResult::Revealed);
        CHECK(g.depth() == 1);
        CHECK(g.tap(1, 0) == TapResult::Descended);
        CHECK(g.depth() == 2);
        CHECK(g.hero().gold == 9);
        CHECK(g.grid().revealedCount() == 1);
    }
    // Clearing the whole board awards a bonus exactly once.
    {
        Game g(1); blank(g);
        Grid &gr = g.mutableGrid();
        // Leave only (1,0) gold and (2,0) stairs hidden/unrevealed; everything else revealed empty.
        for (int y = 0; y < gr.height(); ++y)
            for (int x = 0; x < gr.width(); ++x) gr.setTile(x, y, TileKind::Empty, 0, true);
        gr.setTile(1, 0, TileKind::Gold, 4, false);
        gr.setTile(2, 0, TileKind::Stairs, 0, false);
        CHECK(!g.boardCleared());
        CHECK(g.tap(2, 0) == TapResult::Revealed);   // stairs revealed, gold still there
        CHECK(!g.boardCleared() && g.lastBonus() == 0);
        CHECK(g.tap(1, 0) == TapResult::Revealed);   // gold revealed, not yet collected
        CHECK(!g.boardCleared());
        const int before = g.hero().gold;
        CHECK(g.tap(1, 0) == TapResult::Cleared);    // collecting the last item clears the board
        CHECK(g.lastBonus() == 10 + 5 * 1);
        CHECK(g.hero().gold == before + 4 + g.lastBonus());
        CHECK(g.tap(0, 0) == TapResult::Nothing);    // no second bonus
        CHECK(g.hero().gold == before + 4 + 15);
    }
    // Killing the last monster also clears the board.
    {
        Game g(1); blank(g);
        Grid &gr = g.mutableGrid();
        for (int y = 0; y < gr.height(); ++y)
            for (int x = 0; x < gr.width(); ++x) gr.setTile(x, y, TileKind::Empty, 0, true);
        gr.setTile(3, 3, TileKind::Monster, 3, true);
        CHECK(!g.boardCleared());
        CHECK(g.tap(3, 3) == TapResult::Cleared);   // killing the last monster clears it
    }

    // Shop: opens only after finishing level 5 (and every 5th after), not before.
    {
        Game g(3);
        for (int i = 1; i <= 4; ++i) {
            g.mutableGrid().setTile(0, 0, TileKind::Stairs, 0, true);
            CHECK(g.tap(0, 0) == TapResult::Descended);
            CHECK(!g.shopOpen());   // arriving at depths 2..5
        }
        CHECK(g.depth() == 5);
        g.mutableGrid().setTile(0, 0, TileKind::Stairs, 0, true);
        CHECK(g.tap(0, 0) == TapResult::Descended);
        CHECK(g.depth() == 6 && g.shopOpen());

        // Board taps are ignored while shopping; purchases need 100 gold each.
        CHECK(g.tap(1, 1) == TapResult::Nothing);
        g.mutableHero().gold = 99;
        CHECK(!g.buyDamage() && !g.buyHeal());
        g.mutableHero().gold = 250;
        g.mutableHero().hp = 4;
        CHECK(g.buyDamage());
        CHECK(g.hero().attack == 4 && g.hero().gold == 150);
        CHECK(g.buyHeal());
        CHECK(g.hero().maxHp == 25 && g.hero().hp == 25 && g.hero().gold == 50);
        CHECK(!g.buyDamage());

        // Shop state survives save/load.
        Game r(1);
        CHECK(Game::load(g.serialize(), r));
        CHECK(r.shopOpen() && r.serialize() == g.serialize());

        g.leaveShop();
        CHECK(!g.shopOpen());
        CHECK(!g.buyDamage());  // can't buy outside the shop
        CHECK(g.serialize().find("INKDELVE3") == 0);
    }
    // Purchases are impossible when no shop is open, and v1 saves still load.
    {
        Game g(4);
        g.mutableHero().gold = 500;
        CHECK(!g.buyDamage() && !g.buyHeal());
        std::string v1 = g.serialize();
        // v3 tokens: 0 magic .. 6 gold, 7 kills, 8 totalGold, 9 shop, then w h and tiles.
        // v1 drops tokens 7-9; v2 drops tokens 7-8.
        std::istringstream is(v1);
        std::string tok, v1text, v2text;
        for (int i = 0; is >> tok; ++i) {
            std::string t1 = tok, t2 = tok;
            if (i == 0) { t1 = "INKDELVE1"; t2 = "INKDELVE2"; }
            if (i < 7 || i > 9) v1text += t1 + " ";
            if (i < 7 || i > 8) v2text += t2 + " ";
        }
        Game r(1);
        CHECK(Game::load(v1text, r));
        CHECK(!r.shopOpen() && r.hero().gold == 500 && r.totalGold() == 500 && r.kills() == 0);
        Game r2(1);
        CHECK(Game::load(v2text, r2));
        CHECK(!r2.shopOpen() && r2.hero().gold == 500 && r2.totalGold() == 500);
    }
    // Scoring: floors, kills and total gold acquired; shop spending does not lower it.
    {
        Game g(1); blank(g);
        CHECK(g.score() == 1 * Game::kScorePerFloor);  // floor 1, nothing else yet
        g.mutableGrid().setTile(1, 0, TileKind::Gold, 30, false);
        g.mutableGrid().setTile(2, 0, TileKind::Monster, 3, false);
        CHECK(g.tap(1, 0) == TapResult::Revealed);
        CHECK(g.tap(1, 0) == TapResult::Collected);
        CHECK(g.totalGold() == 30);
        CHECK(g.tap(2, 0) == TapResult::Revealed);
        CHECK(g.tap(2, 0) == TapResult::Killed);
        CHECK(g.kills() == 1);
        CHECK(g.score() == 100 + 10 + 30);
        // Spending gold keeps totalGold (and score) unchanged.
        g.mutableHero().gold = 120;
        const int before = g.score();
        g.mutableGrid().setTile(0, 0, TileKind::Stairs, 0, true);
        CHECK(g.tap(0, 0) == TapResult::Descended);
        CHECK(g.depth() == 2 && g.score() == before + Game::kScorePerFloor);
        // Board-clear bonus counts as gold acquired.
        Game c(1); blank(c);
        Grid &gr = c.mutableGrid();
        for (int y = 0; y < gr.height(); ++y)
            for (int x = 0; x < gr.width(); ++x) gr.setTile(x, y, TileKind::Empty, 0, true);
        gr.setTile(3, 3, TileKind::Monster, 3, true);
        CHECK(c.tap(3, 3) == TapResult::Cleared);
        CHECK(c.totalGold() == c.lastBonus() && c.kills() == 1);
        // Kills, totalGold and score survive save/load.
        Game l(1);
        CHECK(Game::load(c.serialize(), l));
        CHECK(l.score() == c.score() && l.kills() == 1 && l.totalGold() == c.totalGold());
    }
    // High score: keeps the best score and its floor; ties do not replace; bad input rejected.
    {
        HighScore h;
        CHECK(h.score == 0 && h.floor == 0);
        CHECK(h.update(500, 4));
        CHECK(h.score == 500 && h.floor == 4);
        CHECK(!h.update(400, 9));     // lower score, higher floor: not a new best
        CHECK(!h.update(500, 9));     // tie: unchanged
        CHECK(h.score == 500 && h.floor == 4);
        CHECK(h.update(900, 7));
        HighScore r;
        CHECK(HighScore::load(h.serialize(), r));
        CHECK(r.score == 900 && r.floor == 7);
        HighScore keep; keep.update(1, 1);
        CHECK(!HighScore::load("", keep));
        CHECK(!HighScore::load("junk 5 5", keep));
        CHECK(!HighScore::load("INKDELVEHI1 -5 3", keep));
        CHECK(keep.score == 1 && keep.floor == 1);
    }

    // Deeper levels hit harder.
    {
        Game g(5);
        CHECK(g.monsterDamage() == 1);
        for (int i = 0; i < 2; ++i) {
            g.mutableGrid().setTile(0, 0, TileKind::Stairs, 0, true);
            CHECK(g.tap(0, 0) == TapResult::Descended);
        }
        CHECK(g.depth() == 3);
        CHECK(g.monsterDamage() == 2);
    }

    // Save/load round trip preserves hero, depth, tiles and future behavior.
    {
        Game g(42);
        g.mutableHero().gold = 11;
        g.mutableHero().hp = 13;
        g.mutableGrid().setTile(0, 0, TileKind::Empty, 0, true);
        g.mutableGrid().setTile(1, 0, TileKind::Monster, 6, true);
        const std::string saved = g.serialize();

        Game r(1);
        CHECK(Game::load(saved, r));
        CHECK(r.serialize() == saved);
        CHECK(r.hero().gold == 11 && r.hero().hp == 13 && r.depth() == 1);
        CHECK(r.grid().at(1, 0).kind == TileKind::Monster && r.grid().at(1, 0).value == 6);
        // Both continue identically, including the next level generated from the saved seed.
        CHECK(g.tap(1, 0) == r.tap(1, 0));
        CHECK(g.serialize() == r.serialize());
        g.mutableGrid().setTile(0, 0, TileKind::Stairs, 0, true);
        r.mutableGrid().setTile(0, 0, TileKind::Stairs, 0, true);
        CHECK(g.tap(0, 0) == TapResult::Descended && r.tap(0, 0) == TapResult::Descended);
        CHECK(g.serialize() == r.serialize());
    }
    // Malformed or out-of-range saves are rejected and leave the target untouched.
    {
        Game g(7);
        const std::string good = g.serialize();
        Game t(9);
        const std::string before = t.serialize();
        CHECK(!Game::load("", t));
        CHECK(!Game::load("garbage", t));
        CHECK(!Game::load(good.substr(0, good.size() / 2), t));  // truncated
        std::string bad = good;
        bad.replace(0, 9, "INKDELVE9");
        CHECK(!Game::load(bad, t));
        CHECK(t.serialize() == before);
        // Dead hero is not a resumable save.
        Game d(7);
        d.mutableHero().hp = 0;
        CHECK(!Game::load(d.serialize(), t));
        CHECK(t.serialize() == before);
    }

    if (failures == 0) std::printf("All game tests passed\n");
    return failures ? 1 : 0;
}
