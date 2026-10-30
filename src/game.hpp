#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace echovault {

struct Point {
    int x{};
    int y{};
    bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) const { return !(*this == other); }
};

struct Level {
    std::string title;
    std::string subtitle;
    std::vector<std::string> grid;
    Point start;
    Point exit;
};

struct Echo {
    std::vector<Point> recording;
    Point at(std::size_t tick) const;
};

enum class Result { Ignored, Moved, Blocked, Rewound, Reset, Won, LevelAdvanced, Finished };

class Game {
public:
    explicit Game(std::vector<Level> levels = makeLevels());

    Result command(char key);
    bool isDoorOpen(char door, std::size_t atTick, Point playerPosition) const;
    char tile(Point p) const;
    bool finished() const { return finished_; }
    bool won() const { return won_; }
    std::size_t levelIndex() const { return levelIndex_; }
    std::size_t ticks() const { return ticks_; }
    std::size_t rewinds() const { return rewinds_; }
    Point player() const { return player_; }
    const Level& level() const { return levels_.at(levelIndex_); }
    const std::vector<Echo>& echoes() const { return echoes_; }
    const std::string& message() const { return message_; }
    static std::vector<Level> makeLevels();

private:
    void resetLevel(bool forgetEchoes);
    Result move(char key);
    std::vector<Level> levels_;
    std::size_t levelIndex_{};
    Point player_{};
    std::size_t ticks_{};
    std::size_t rewinds_{};
    std::vector<Point> recording_;
    std::vector<Echo> echoes_;
    std::string message_;
    bool won_{};
    bool finished_{};
};

} // namespace echovault
