#include "game.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using echovault::Game;
using echovault::Point;
using echovault::Result;

namespace {
constexpr const char* RESET = "\033[0m";
constexpr const char* TEAL = "\033[96m";
constexpr const char* GREEN = "\033[92m";
constexpr const char* MAGENTA = "\033[95m";
constexpr const char* ORANGE = "\033[93m";
constexpr const char* RED = "\033[91m";
constexpr const char* GRAY = "\033[90m";
constexpr const char* BOLD = "\033[1m";

class Terminal {
public:
    explicit Terminal(bool color) : color_(color) {
#ifdef _WIN32
        HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD flags{};
        if (output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &flags)) {
            output_ = output;
            originalMode_ = flags;
            hasMode_ = true;
            SetConsoleMode(output_, flags | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
        SetConsoleOutputCP(CP_UTF8);
#else
        if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &saved_) == 0) {
            termios raw = saved_;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) rawEnabled_ = true;
        }
#endif
        if (color_) std::cout << "\033[?25l\033[2J\033[H" << std::flush;
    }
    ~Terminal() {
        if (color_) std::cout << "\033[?25h\033[0m\n" << std::flush;
#ifdef _WIN32
        if (hasMode_) SetConsoleMode(output_, originalMode_);
#else
        if (rawEnabled_) tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
#endif
    }
    char readKey() const {
#ifdef _WIN32
        int key = _getch();
        if (key == 0 || key == 224) {
            switch (_getch()) {
                case 72: return 'w';
                case 80: return 's';
                case 75: return 'a';
                case 77: return 'd';
                default: return '\0';
            }
        }
        return static_cast<char>(key);
#else
        char key{};
        if (!std::cin.get(key)) return 'q';
        if (key == '\033') {
            char a{}, b{};
            if (std::cin.get(a) && a == '[' && std::cin.get(b)) {
                switch (b) {
                    case 'A': return 'w';
                    case 'B': return 's';
                    case 'C': return 'd';
                    case 'D': return 'a';
                    default: break;
                }
            }
            return '\0';
        }
        return key;
#endif
    }
    bool color() const { return color_; }
private:
    bool color_;
#ifdef _WIN32
    HANDLE output_{};
    DWORD originalMode_{};
    bool hasMode_{};
#else
    termios saved_{};
    bool rawEnabled_{};
#endif
};

std::string colored(const std::string& word, const char* tint, bool enabled) {
    return enabled ? std::string(tint) + word + RESET : word;
}

std::string cell(const Game& game, Point position, bool color) {
    if (position == game.player()) return colored("@>", BOLD, color);
    int seen = 0;
    int earliest = 0;
    for (std::size_t i = 0; i < game.echoes().size(); ++i) {
        if (game.echoes()[i].at(game.ticks()) == position) {
            ++seen;
            if (!earliest) earliest = static_cast<int>(i + 1);
        }
    }
    if (seen > 1) return colored("**", MAGENTA, color);
    if (seen == 1) return colored(std::to_string(earliest) + "*", MAGENTA, color);
    const char tile = game.tile(position);
    if (tile == '#') return colored("##", GRAY, color);
    if (tile >= 'A' && tile <= 'C') {
        return game.isDoorOpen(tile, game.ticks(), game.player())
                   ? colored(std::string(1, tile) + "/", GREEN, color)
                   : colored(std::string(1, tile) + "X", RED, color);
    }
    if (tile >= 'a' && tile <= 'c') return colored(std::string(1, tile) + "_", ORANGE, color);
    if (tile == 'E') return colored("<>", GREEN, color);
    if (tile == 'S') return colored("S ", TEAL, color);
    return colored(". ", GRAY, color);
}

std::string doors(const Game& game) {
    std::ostringstream out;
    for (char door = 'A'; door <= 'C'; ++door) {
        const auto& grid = game.level().grid;
        const bool exists = std::any_of(grid.begin(), grid.end(), [door](const std::string& row) {
            return row.find(door) != std::string::npos;
        });
        if (exists) out << door << ':' << (game.isDoorOpen(door, game.ticks(), game.player()) ? "OPEN " : "LOCKED ");
    }
    return out.str();
}

void draw(const Game& game, bool color, bool help, const std::string& banner = "") {
    if (color) std::cout << "\033[H\033[2J";
    std::cout << "\n  " << colored(" E C H O V A U L T ", TEAL, color)
              << "     " << colored("TIME-LOOP HEIST // C++17", MAGENTA, color) << "\n";
    std::cout << "  -----------------------------------------------------------------------------\n";
    std::cout << "  " << colored(game.level().title, BOLD, color) << "    |    " << game.level().subtitle << "\n\n";

    std::vector<std::string> sidebar{
        " [  STATUS  ]",
        " Level  " + std::to_string(game.levelIndex() + 1) + "/3",
        " Tick   " + std::to_string(game.ticks()),
        " Echoes " + std::to_string(game.echoes().size()) + " / 8",
        " Loops  " + std::to_string(game.rewinds()),
        "",
        " [  GATES  ]",
        " " + doors(game),
        "",
        " [  CONTROLS  ]",
        " WASD / ARROWS   move",
        " .               wait",
        " R               rewind + clone",
        " C               erase echoes",
        " H               toggle help",
        " Q               quit",
    };
    const auto& grid = game.level().grid;
    for (std::size_t y = 0; y < grid.size(); ++y) {
        std::cout << "  ";
        for (std::size_t x = 0; x < grid[y].size(); ++x) {
            std::cout << cell(game, Point{static_cast<int>(x), static_cast<int>(y)}, color);
        }
        if (y < sidebar.size()) std::cout << "     " << sidebar[y];
        std::cout << '\n';
    }
    if (grid.size() < sidebar.size()) {
        for (std::size_t y = grid.size(); y < sidebar.size(); ++y) {
            std::cout << std::string(grid.front().size() * 2 + 7, ' ') << sidebar[y] << '\n';
        }
    }
    std::cout << "\n  " << colored("@>", BOLD, color) << " you   "
              << colored("1*", MAGENTA, color) << " previous self   "
              << colored("a_", ORANGE, color) << " switch   "
              << colored("AX", RED, color) << " locked   "
              << colored("A/", GREEN, color) << " open   "
              << colored("<>", GREEN, color) << " exit\n";
    std::cout << "\n  " << (banner.empty() ? game.message() : banner) << "\n";
    if (help) {
        std::cout << "\n  " << colored("HOW TIME WORKS", TEAL, color) << '\n'
                  << "  Move onto a colored switch, then press R. Your recorded route replays as\n"
                  << "  an echo from tick 0. It stays at its last tile after the recording ends.\n"
                  << "  In the next loop, cross the door while the echo holds its matching switch.\n"
                  << "  Make another echo for each additional gate. Press . to synchronize time.\n";
    }
    if (game.won()) std::cout << colored("\n  >> LEVEL COMPLETE! PRESS N FOR THE NEXT VAULT <<\n", GREEN, color);
    if (game.finished()) std::cout << colored("\n  >> ALL THREE VAULTS CLEARED. YOU OUTSMARTED TIME. <<\n", GREEN, color);
    std::cout << std::flush;
}

void rewindAnimation(const Game& game, bool color, bool help, bool animate) {
    if (!animate) return;
    for (int i = 3; i >= 1; --i) {
        draw(game, color, help, "<< REWIND COMPLETE / ECHO REPLAY IN " + std::to_string(i) + " >>");
        std::this_thread::sleep_for(std::chrono::milliseconds(110));
    }
}

void usage(const char* program) {
    std::cout << "EchoVault - a three-level C++ time-loop puzzle\n\n"
              << "Usage: " << program << " [--no-color] [--no-animation] [--demo] [--script MOVES]\n"
              << "WASD or arrow keys move; . wait; R record and rewind; C restart; H help; Q quit.\n"
              << "In --script mode, command letters run without opening an interactive terminal.\n"
              << "In --demo mode, the first level is played as an animated example.\n";
}
} // namespace

int main(int argc, char** argv) {
    bool color = true;
    bool animate = true;
    bool scripted = false;
    bool demo = false;
    std::string script;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") { usage(argv[0]); return 0; }
        if (arg == "--no-color") color = false;
        else if (arg == "--no-animation") animate = false;
        else if (arg == "--demo") demo = true;
        else if (arg == "--script" && i + 1 < argc) { scripted = true; script = argv[++i]; }
        else { std::cerr << "Unknown or incomplete argument: " << arg << '\n'; return 2; }
    }

    Game game;
    if (scripted) {
        for (char ch : script) game.command(ch);
        draw(game, false, false);
        return 0;
    }
    Terminal terminal(color);
    bool help = false;
    draw(game, terminal.color(), help);
    if (demo) {
        const std::string demonstration = "ddssssr" + std::string("ss") + std::string(14, 'd');
        for (char action : demonstration) {
            const Result result = game.command(action);
            if (result == Result::Rewound) rewindAnimation(game, terminal.color(), help, animate);
            draw(game, terminal.color(), help);
            if (animate) std::this_thread::sleep_for(std::chrono::milliseconds(115));
        }
        return game.won() ? 0 : 1;
    }
    while (!game.finished()) {
        const char key = terminal.readKey();
        const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
        if (lower == 'q') break;
        if (lower == 'h') { help = !help; draw(game, terminal.color(), help); continue; }
        const Result result = game.command(key);
        if (result == Result::Rewound) rewindAnimation(game, terminal.color(), help, animate);
        draw(game, terminal.color(), help);
    }
    return 0;
}
