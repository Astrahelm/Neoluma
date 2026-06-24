#include "Paths.hpp"
#include <stdexcept>

#if _WIN32
    #include <windows.h>
#elif __APPLE__ && __MACH__
    #include <mach-o/dyld.h>
    #include <limits.h>
    #include <unistd.h>
#else
    #include <limits.h>
    #include <unistd.h>
#endif

#include <cstdlib>
#include <string>

#if _WIN32
static std::string getEnvVar(const char* name){
    char* buf = nullptr;
    size_t len = 0;
    errno_t err = _dupenv_s(&buf, &len, name);
    if (err != 0 || !buf) return "";

    std::string value(buf);
    free(buf);
    return value;
}
#else
static std::string getEnvVar(const char* name){
    const char* v = std::getenv(name);
    return v ? std::string(v) : "";
}
#endif

static std::string normalizeSlashes(std::string s){
    for (auto& c : s ) if (c == '\\') c = '/';
    return s;
}

std::string Paths::join(const std::string& a, const std::string& b){
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a.back() == '/' || a.back() == '\\') return a + b;
    return a + '/' + b;
}

std::string Paths::executablePath(){
#if _WIN32
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (len == 0 || len == MAX_PATH) throw std::runtime_error("Paths::executablePath(): GetModuleFileNameA failed");
    return normalizeSlashes(std::string(buf, len));
#elif __APPLE__ && __MACH__
    uint32_t size = 0;
    _NSGetExecutablePath(NULL, &size);
    std::string out(size, '\0');
    if (_NSGetExecutablePath(out.data(), &size) != 0) throw std::runtime_error("Paths::executablePath(): _NSGetExecutablePath failed");
    out = std::string(out.c_str());
    char resolvedPath[PATH_MAX];
    if (realpath(out.c_str(), resolvedPath)) return std::string(resolvedPath);
    return out;
#else
    char resolvedPath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", resolvedPath, sizeof(resolvedPath)-1);
    if (len <= 0) throw std::runtime_error("Paths::executablePath(): readlink failed");
    resolvedPath[len] = '\0';
    return std::string(resolvedPath);
#endif
}

// Gets Neoluma's installation folder
std::string Paths::rootDir(){
    auto p = normalizeSlashes(executablePath());

    // remove executable name
    auto pos = p.find_last_of('/');
    if (pos == std::string::npos) return ".";
    p = p.substr(0, pos);

    // remove "bin"
    pos = p.find_last_of('/');
    if (pos == std::string::npos) return ".";
    return p.substr(0, pos);
}

// Gets Neoluma's data folder
std::string Paths::dataDir()
{
    return rootDir() + "/share";
}

std::string Paths::userDataDir(){
#if _WIN32
    auto local = getEnvVar("LOCALAPPDATA");
    if (!local.empty()) return join(normalizeSlashes(local), "Neoluma");

    auto app = getEnvVar("APPDATA");
    if (!app.empty()) return join(normalizeSlashes(app), "Neoluma");

    return join(rootDir(), "userData");

#elif __APPLE__ && __MACH__
    auto home = getEnvVar("HOME");
    if (!home.empty())
        return normalizeSlashes(home) + "/Library/Application Support/Neoluma";

    return join(rootDir(), "userData");

#else
    auto xdg = getEnvVar("XDG_DATA_HOME");
    if (!xdg.empty()) return join(normalizeSlashes(xdg), "neoluma");

    auto home = getEnvVar("HOME");
    if (!home.empty())
        return normalizeSlashes(home) + "/.local/share/neoluma";

    return join(rootDir(), "userData");
#endif
}