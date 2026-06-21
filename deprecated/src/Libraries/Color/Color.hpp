#pragma once

#include <iostream>

namespace Color {
    std::string TextRGB(uint8_t r, uint8_t g, uint8_t b);

    std::string BackgroundRGB(uint8_t r, uint8_t g, uint8_t b);

    std::string TextHex(const std::string& hex);

    std::string BackgroundHex(const std::string& hex);

    namespace Effect {
        constexpr std::string BoldOn       = "\033[1m";
        constexpr std::string DimOn        = "\033[2m";
        constexpr std::string UnderlineOn  = "\033[4m";
        constexpr std::string BlinkOn      = "\033[5m";
        constexpr std::string ReverseOn    = "\033[7m";
        constexpr std::string HideOn       = "\033[8m";

        constexpr std::string BoldOff      = "\033[21m";
        constexpr std::string DimOff       = "\033[22m";
        constexpr std::string UnderlineOff = "\033[24m";
        constexpr std::string BlinkOff     = "\033[25m";
        constexpr std::string ReverseOff   = "\033[27m";
        constexpr std::string HideOff      = "\033[28m";
    }

    constexpr std::string Reset = "\033[0m";
}
