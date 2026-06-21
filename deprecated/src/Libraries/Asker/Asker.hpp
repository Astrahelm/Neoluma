#pragma once
/* 
* Asker is an internal Neoluma library for handling CLI input
*/
#include <iostream>
#include <print>
#include "../Color/Color.hpp"
enum Key { NONE, UP, DOWN, ENTER, BACKSPACE };

#ifdef _WIN32
#include <conio.h>
Key getKey();
void enableRaw(); // do nothing on windows
void disableRaw();
#else
#include <termios.h>
#include <unistd.h>
static struct termios term;
void enableRaw();
void disableRaw();
Key getKey();
#endif

void clearBlock(int lines);

namespace asker {
    constexpr const char* Clear = "\033[0m\033[2J\033[H";

    std::string input(const std::string& question, bool required=false);

    bool confirm(const std::string& question);

    template <size_t n>
    std::string selectList(const std::string& question, const std::string (&options)[n]) {
        enableRaw();

        int pos = 0;
        Key key = NONE;
        int lines = n + 2;

        while (true) {
            clearBlock(lines);

            std::cout << Color::TextHex("#75b5ff") << ">> " << question << '\n';

            for (int i = 0; i < n; i++) {
                if (i == pos) { 
                    std::cout << Color::TextHex("#00ff48ffff") << "> " << Color::TextHex("#ff28e6") << options[i] << '\n';
                }
                else { 
                    std::cout << Color::TextHex("#ff28e6") << "  " << options[i] << '\n';
                }
            }

            key = getKey();
            if (key == UP && pos > 0) pos--;
            else if (key == DOWN && pos < n - 1) pos++;
            else if (key == ENTER) break;
        }

        disableRaw();
        std::cout << Color::Reset << "\n";
        return options[pos];
    }
};