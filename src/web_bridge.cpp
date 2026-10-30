// Browser-facing adapter. The game rules remain exclusively in game.cpp.
#include "game.hpp"
#include <emscripten/bind.h>
#include <sstream>
#include <string>

namespace {
echovault::Game game;

std::string quote(const std::string& input) {
    std::string out = "\"";
    constexpr char digits[] = "0123456789abcdef";
    for (unsigned char c : input) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    out += "\\u00";
                    out += digits[c >> 4];
                    out += digits[c & 0xf];
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
    return out + '"';
}

std::string point(const echovault::Point& p) {
    return "[" + std::to_string(p.x) + "," + std::to_string(p.y) + "]";
}

std::string stateJson() {
    std::ostringstream json;
    json << "{\"levelIndex\":" << game.levelIndex()
         << ",\"levelCount\":" << echovault::Game::makeLevels().size()
         << ",\"title\":" << quote(game.level().title)
         << ",\"subtitle\":" << quote(game.level().subtitle)
         << ",\"grid\":[";
    bool first = true;
    for (const auto& line : game.level().grid) {
        if (!first) json << ',';
        first = false;
        json << quote(line);
    }
    json << "],\"player\":" << point(game.player())
         << ",\"exit\":" << point(game.level().exit)
         << ",\"ticks\":" << game.ticks()
         << ",\"rewinds\":" << game.rewinds()
         << ",\"won\":" << (game.won() ? "true" : "false")
         << ",\"finished\":" << (game.finished() ? "true" : "false")
         << ",\"message\":" << quote(game.message())
         << ",\"echoes\":[";
    first = true;
    for (const auto& echo : game.echoes()) {
        if (!first) json << ',';
        first = false;
        json << "{\"at\":" << point(echo.at(game.ticks())) << ",\"path\":[";
        bool firstPoint = true;
        for (const auto& pos : echo.recording) {
            if (!firstPoint) json << ',';
            firstPoint = false;
            json << point(pos);
        }
        json << "]}";
    }
    json << "],\"gates\":{";
    for (char gate = 'A'; gate <= 'C'; ++gate) {
        if (gate != 'A') json << ',';
        json << quote(std::string(1, gate)) << ':'
             << (game.isDoorOpen(gate, game.ticks(), game.player()) ? "true" : "false");
    }
    return json.str() + "}}";
}

int performCommand(const std::string& key) {
    if (key.size() != 1) return static_cast<int>(echovault::Result::Ignored);
    return static_cast<int>(game.command(key[0]));
}
}

EMSCRIPTEN_BINDINGS(echovault_web) {
    emscripten::function("getState", &stateJson);
    emscripten::function("command", &performCommand);
}
