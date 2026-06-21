#include "Asker.hpp"

#include <string>

#ifdef _WIN32
#include <conio.h>

Key getKey() {
    int c = _getch();
    if (c == 13) return ENTER;
    if (c == 8) return BACKSPACE;
    if (c == 224 || c == 0) {
        c = _getch();
        if (c == 72) return UP;
        if (c == 80) return DOWN;
    }
    return NONE;
}

void enableRaw() {} // do nothing on windows
void disableRaw() {}

#else
#include <termios.h>
#include <unistd.h>
void enableRaw() {
    tcgetattr(STDIN_FILENO, &term);
    struct termios raw = term;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void disableRaw() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

Key getKey() {
    int c = getchar();
    if (c == '\n') return ENTER;
    if (c == 127) return BACKSPACE;
    if (c == 27) {
        char sequence[2];
        if (read(STDIN_FILENO, sequence, 2) == 2 && sequence[0] == '[') {
            if (sequence[1] == 'A') return UP;
            if (sequence[1] == 'B') return DOWN;
        }
    }
    return NONE;
}
#endif

void clearBlock(int lines) {
    for (int i = 0; i < lines; i++) {
        std::cout << "\033[1A";  // up
        std::cout << "\033[2K";  // clear whole line
        std::cout << "\r";
    }
}

namespace asker {
    std::string input(const std::string& question, bool required) {
        std::cout << Color::TextHex("#75b5ff") << ">> " << question << ' ' << Color::TextHex("#ff28e6");
        std::string response;
        while (std::getline(std::cin, response)) {
            if (required && response.length() == 0) {}
            else break;
        }
        std::cout << Color::Reset;
        return response;
    }

    bool confirm(const std::string& question) {
        bool result = true;
        std::string response;
        response = input(question + Color::TextHex("#e84b85") + " (y/n) " + Color::TextHex("#ff28e6"));
        if (response[0] != 'y' && response[0] != 'Y' && response[0] != '\0' ) result = false;
        std::cout << Color::Reset;
        return result;
    }
}