#include "game.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace echovault {
namespace {
Level makeLevel(std::string title, std::string subtitle, int width, int height,
                std::vector<int> walls, std::vector<Point> doors,
                std::vector<Point> plates, Point start, Point exit) {
    if (walls.size() != doors.size() || walls.size() != plates.size()) {
        throw std::invalid_argument("Every barrier must have a door and switch");
    }
    Level result{std::move(title), std::move(subtitle),
                 std::vector<std::string>(static_cast<std::size_t>(height), std::string(static_cast<std::size_t>(width), '.')),
                 start, exit};
    for (int x = 0; x < width; ++x) {
        result.grid.front()[x] = '#';
        result.grid.back()[x] = '#';
    }
    for (int y = 0; y < height; ++y) {
        result.grid[y].front() = '#';
        result.grid[y].back() = '#';
    }
    for (std::size_t i = 0; i < walls.size(); ++i) {
        const int x = walls[i];
        for (int y = 1; y < height - 1; ++y) result.grid[y][x] = '#';
        result.grid[doors[i].y][doors[i].x] = static_cast<char>('A' + i);
        result.grid[plates[i].y][plates[i].x] = static_cast<char>('a' + i);
    }
    result.grid[start.y][start.x] = 'S';
    result.grid[exit.y][exit.x] = 'E';
    return result;
}
} // namespace

std::vector<Level> Game::makeLevels() {
    return {
        makeLevel("01 / THE FIRST ECHO", "A past self can hold a switch that you cannot.",
                  17, 9, {8}, {{8, 3}}, {{3, 5}}, {1, 1}, {15, 3}),
        makeLevel("02 / TWO LIVES", "One echo opens A. The next can reach B.",
                  21, 11, {7, 14}, {{7, 3}, {14, 7}}, {{3, 5}, {10, 8}}, {1, 1}, {19, 7}),
        makeLevel("03 / PARADOX ENGINE", "Chain three timelines to cross three gates.",
                  27, 11, {6, 12, 19}, {{6, 2}, {12, 5}, {19, 7}},
                  {{3, 8}, {9, 8}, {16, 8}}, {1, 1}, {25, 7}),
    };
}

Point Echo::at(std::size_t tick) const {
    if (recording.empty()) throw std::logic_error("Empty echo recording");
    return recording[std::min(tick, recording.size() - 1)];
}

Game::Game(std::vector<Level> levels) : levels_(std::move(levels)) {
    if (levels_.empty()) throw std::invalid_argument("At least one level is required");
    for (const Level& lvl : levels_) {
        if (lvl.grid.empty() || lvl.grid.front().empty()) throw std::invalid_argument("Empty map");
        for (const std::string& row : lvl.grid) {
            if (row.size() != lvl.grid.front().size()) throw std::invalid_argument("Map must be rectangular");
        }
    }
    resetLevel(true);
}

void Game::resetLevel(bool forgetEchoes) {
    player_ = level().start;
    ticks_ = 0;
    recording_ = {player_};
    won_ = false;
    if (forgetEchoes) {
        echoes_.clear();
        rewinds_ = 0;
    }
    message_ = forgetEchoes ? "Get to E. Record your route, then rewind with R." : "Time rewound. Your echoes remember.";
}

char Game::tile(Point p) const {
    if (p.x < 0 || p.y < 0 || p.y >= static_cast<int>(level().grid.size()) ||
        p.x >= static_cast<int>(level().grid.front().size())) return '#';
    return level().grid[static_cast<std::size_t>(p.y)][static_cast<std::size_t>(p.x)];
}

bool Game::isDoorOpen(char door, std::size_t atTick, Point playerPosition) const {
    if (door < 'A' || door > 'C') return false;
    const char plate = static_cast<char>(std::tolower(static_cast<unsigned char>(door)));
    if (tile(playerPosition) == plate) return true;
    for (const Echo& echo : echoes_) {
        if (tile(echo.at(atTick)) == plate) return true;
    }
    return false;
}

Result Game::move(char key) {
    Point target = player_;
    switch (key) {
        case 'w': --target.y; break;
        case 's': ++target.y; break;
        case 'a': --target.x; break;
        case 'd': ++target.x; break;
        case '.': break;
        default: return Result::Ignored;
    }
    const std::size_t nextTick = ticks_ + 1;
    const char destination = tile(target);
    const bool lockedDoor = destination >= 'A' && destination <= 'C' &&
                             !isDoorOpen(destination, nextTick, target);
    const bool blocked = destination == '#' || lockedDoor;
    if (!blocked) player_ = target;
    ++ticks_;
    recording_.push_back(player_);
    if (blocked) {
        message_ = lockedDoor ? "LOCKED! An echo must stand on its matching plate." : "A wall blocks the way.";
        return Result::Blocked;
    }
    if (player_ == level().exit) {
        won_ = true;
        message_ = "VAULT OPEN! Press N to continue.";
        return Result::Won;
    }
    message_ = key == '.' ? "Waiting ... your echoes keep moving." : "Timeline advanced.";
    return Result::Moved;
}

Result Game::command(char key) {
    const char keyLower = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
    if (finished_) return Result::Finished;
    if (won_) {
        if (keyLower == 'n' || keyLower == '\n' || keyLower == '\r') {
            if (levelIndex_ + 1 == levels_.size()) {
                finished_ = true;
                message_ = "All timelines restored. Congratulations!";
                return Result::Finished;
            }
            ++levelIndex_;
            resetLevel(true);
            return Result::LevelAdvanced;
        }
        return Result::Ignored;
    }
    if (keyLower == 'c') {
        resetLevel(true);
        return Result::Reset;
    }
    if (keyLower == 'r') {
        if (ticks_ == 0) {
            message_ = "Make at least one move before recording an echo.";
            return Result::Ignored;
        }
        if (echoes_.size() >= 8) {
            message_ = "Echo limit reached (8). Press C to start over.";
            return Result::Ignored;
        }
        echoes_.push_back(Echo{recording_});
        ++rewinds_;
        resetLevel(false);
        return Result::Rewound;
    }
    return move(keyLower);
}

} // namespace echovault
