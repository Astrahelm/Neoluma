/*
 * Path is an internal Neoluma library for handling paths near Neoluma executable installation.
 * Really, i made it just for that.
 */

#pragma once
#include <string>

namespace Paths {
    std::string executablePath();

    std::string rootDir();
    std::string dataDir();
    std::string userDataDir();

    std::string join(const std::string& a, const std::string& b);
};