#include "game.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using echovault::Game;
using echovault::Point;
using echovault::Result;

namespace {
int assertions = 0;
void require(bool value, const std::string& message) {
    ++assertions;
    if (!value) throw std::runtime_error(message);
}
void play(Game& game, const std::string& keys) {
    for (char key : keys) game.command(key);
}
void testMaps() {
    Game game;
    require(game.level().grid.size() == 9, "level 1 height");
    require(game.tile({-1, 0}) == '#', "out-of-bounds treated as wall");
    require(game.tile({8, 3}) == 'A', "door A on the map");
    require(game.tile({3, 5}) == 'a', "plate a on the map");
}
void testWallsAndLockedDoor() {
    Game game;
    require(game.command('w') == Result::Blocked, "wall collision");
    require(game.player() == (Point{1, 1}), "blocked movement does not teleport player");
    play(game, "ssdddddd");
    require(game.player() == (Point{7, 3}), "approaches locked gate");
    require(game.command('d') == Result::Blocked, "closed door blocks movement");
    require(game.player() == (Point{7, 3}), "door never crossed without plate");
}
void testEchoPlaybackAndFirstWin() {
    Game game;
    play(game, "ddssss");
    require(game.player() == (Point{3, 5}), "player steps on switch a");
    require(game.command('r') == Result::Rewound, "rewind creates echo");
    require(game.echoes().size() == 1, "exactly one echo");
    require(game.echoes()[0].at(0) == (Point{1, 1}), "echo starts at level origin");
    require(game.echoes()[0].at(300) == (Point{3, 5}), "echo holds last position indefinitely");
    play(game, "ssddddddd");
    require(game.player() == (Point{8, 3}), "echo unlocks A door");
    require(game.isDoorOpen('A', game.ticks(), game.player()), "door open while echo holds switch");
    play(game, "ddddddd");
    require(game.won(), "first puzzle is solvable");
    require(game.command('n') == Result::LevelAdvanced, "advance level");
    require(game.levelIndex() == 1 && game.echoes().empty(), "fresh level resets tapes");
}
void testFullLevelTwoWin() {
    Game game;
    play(game, "ddssssr");
    play(game, "ss" + std::string(14, 'd')); // reach level-one exit
    require(game.won(), "first exit reached");
    game.command('n');
    play(game, "ddssssr");
    play(game, "ss" + std::string(9, 'd') + std::string(5, 's'));
    require(game.player() == (Point{10, 8}), "second switch reached");
    game.command('r');
    play(game, "ss" + std::string(12, 'd') + std::string(4, 's') + std::string(6, 'd'));
    require(game.won(), "level 2 solvable with two echoes");
    game.command('n');
    require(game.levelIndex() == 2, "advanced to level 3");
}
void testThirdLevelAndEnding() {
    Game game;
    play(game, "ddssssr");
    play(game, "ss" + std::string(14, 'd'));
    require(game.won(), "first level won");
    game.command('n');
    play(game, "ddssssr");
    play(game, "ss" + std::string(9, 'd') + std::string(5, 's') + "r");
    play(game, "ss" + std::string(12, 'd') + std::string(4, 's') + std::string(6, 'd'));
    require(game.won(), "second level won");
    game.command('n');
    play(game, "dd" + std::string(7, 's') + "r");
    play(game, "....s" + std::string(8, 'd') + std::string(6, 's') + "r");
    play(game, "....s" + std::string(8, 'd') + std::string(6, 's') + "www" + std::string(7, 'd') + std::string(3, 's') + "r");
    require(game.echoes().size() == 3, "three independent timelines");
    play(game, "....s" + std::string(8, 'd') + std::string(6, 's') + "www" + std::string(6, 'd') + "ss" + std::string(10, 'd'));
    require(game.won(), "all three gates can be crossed");
    require(game.command('n') == Result::Finished, "full game completion");
}
void testResetAndInput() {
    Game game;
    require(game.command('r') == Result::Ignored, "empty rewind ignored");
    require(game.command('x') == Result::Ignored, "unexpected key ignored");
    play(game, "ddssssr");
    require(game.echoes().size() == 1, "ghost exists before reset");
    require(game.command('c') == Result::Reset, "reset key works");
    require(game.echoes().empty() && game.ticks() == 0, "reset clears echo state");
}
} // namespace

int main() {
    try {
        testMaps();
        testWallsAndLockedDoor();
        testEchoPlaybackAndFirstWin();
        testFullLevelTwoWin();
        testThirdLevelAndEnding();
        testResetAndInput();
        std::cout << "PASS: " << assertions << " assertions across 6 scenarios\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
}
