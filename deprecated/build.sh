#!/usr/bin/env bash
set -euo pipefail
trap 'echo -e "\n${ERR} Failed at line ${LINENO}: ${BASH_COMMAND}"' ERR

ESC=$'\033'
INFO="${ESC}[38;2;232;75;133m[INFO]${ESC}[0m"
OK="${ESC}[38;2;117;255;135m[SUCCESS]${ESC}[0m"
ERR="${ESC}[38;2;255;80;80m[ERROR]${ESC}[0m"
DONE="${ESC}[38;2;117;255;135m[DONE]${ESC}[0m"

# ============================================================
# Usage:
#   ./build.sh [release|debug] [neoluma|payload|installer|all] [--no-install]
#
# Examples:
#   ./build.sh
#   ./build.sh release all
#   ./build.sh debug payload
#   ./build.sh release installer --no-install
# ============================================================

MODE="${1:-release}"
TARGET="${2:-neoluma}"
TARGET_LC="${TARGET,,}"

AUTO_INSTALL=1
for arg in "$@"; do
  [[ "$arg" == "--no-install" ]] && AUTO_INSTALL=0
done

CFG_PRESET="release-ninja"
BUILD_PRESET="release"
CONFIG="Release"
if [[ "${MODE,,}" == "debug" ]]; then
  CFG_PRESET="debug-ninja"
  BUILD_PRESET="debug"
  CONFIG="Debug"
fi

# ---- Toolchain: clang-22 + libc++ ----
export CC="clang-22"
export CXX="clang++-22"
export CXXFLAGS="${CXXFLAGS:-} -stdlib=libc++"
export LDFLAGS="${LDFLAGS:-} -stdlib=libc++"

have_cmd() { command -v "$1" >/dev/null 2>&1; }
is_wsl() { grep -qi microsoft /proc/version 2>/dev/null; }

PM=""
detect_pm() {
  if have_cmd apt-get; then PM="apt"
  elif have_cmd dnf; then PM="dnf"
  elif have_cmd pacman; then PM="pacman"
  elif have_cmd zypper; then PM="zypper"
  else PM=""
  fi
}

pkg_installed() {
  local pkg="$1"
  case "$PM" in
    apt)
      have_cmd dpkg-query || return 1
      dpkg-query -W -f='${Status}\n' "$pkg" 2>/dev/null | grep -q "install ok installed"
      ;;
    dnf|zypper)
      have_cmd rpm || return 1
      rpm -q "$pkg" >/dev/null 2>&1
      ;;
    pacman)
      have_cmd pacman || return 1
      pacman -Q "$pkg" >/dev/null 2>&1
      ;;
    *) return 1 ;;
  esac
}
pm_install() {
  local pkgs=("$@")
  if (( AUTO_INSTALL == 0 )); then
    echo "${ERR} Missing packages (auto-install disabled):"
    printf "   %s\n" "${pkgs[@]}"
    exit 1
  fi
  if [[ -z "$PM" ]]; then
    echo "${ERR} No supported package manager (apt/dnf/pacman/zypper). Install manually:"
    printf "   %s\n" "${pkgs[@]}"
    exit 1
  fi

  echo "${INFO} Installing via ${PM}:"
  printf "   %s\n" "${pkgs[@]}"

  case "$PM" in
    apt)
      sudo apt-get update
      sudo apt-get install -y "${pkgs[@]}"
      ;;
    dnf)
      sudo dnf install -y "${pkgs[@]}"
      ;;
    pacman)
      sudo pacman -Sy --noconfirm "${pkgs[@]}"
      ;;
    zypper)
      sudo zypper --non-interactive install "${pkgs[@]}"
      ;;
  esac
}

ensure_pkgs() {
  local desired=("$@")
  local missing=()

  for p in "${desired[@]}"; do
    [[ -z "$p" ]] && continue
    if ! pkg_installed "$p"; then
      missing+=("$p")
    fi
  done

  if (( ${#missing[@]} > 0 )); then
    pm_install "${missing[@]}"
  fi
}

pkgs_cpp_for_pm() {
  case "$PM" in
    apt) cat <<'EOF'
ninja-build
build-essential
pkg-config
clang-22
lld-22
llvm-22-dev
libc++-22-dev
libc++abi-22-dev
zlib1g-dev
libzstd-dev
libcurl4-openssl-dev
libedit-dev
libxml2-dev
EOF
    ;;
    dnf) cat <<'EOF'
ninja-build
gcc-c++
pkgconf-pkg-config
clang
lld
llvm-devel
libcxx-devel
libcxxabi-devel
zlib-devel
libzstd-devel
libcurl-devel
libedit-devel
libxml2-devel
EOF
    ;;
    pacman) cat <<'EOF'
ninja
base-devel
pkgconf
clang
lld
llvm
libc++
zlib
zstd
curl
libedit
libxml2
EOF
    ;;
    zypper) cat <<'EOF'
ninja
patterns-devel-base-devel_basis
pkg-config
clang
lld
llvm-devel
libc++-devel
libc++abi-devel
zlib-devel
libzstd-devel
libcurl-devel
libedit-devel
libxml2-devel
EOF
    ;;
  esac
}

pkgs_tauri_for_pm() {
  case "$PM" in
    apt) cat <<'EOF'
nodejs
npm
libglib2.0-dev
libgtk-3-dev
libwebkit2gtk-4.1-dev
libappindicator3-dev
librsvg2-dev
patchelf
EOF
    ;;
    dnf) cat <<'EOF'
nodejs
npm
glib2-devel
gtk3-devel
webkit2gtk4.1-devel
libappindicator-gtk3-devel
librsvg2-devel
patchelf
EOF
    ;;
    pacman) cat <<'EOF'
nodejs
npm
glib2
gtk3
webkit2gtk-4.1
libappindicator-gtk3
librsvg
patchelf
EOF
    ;;
    zypper) cat <<'EOF'
nodejs
npm
glib2-devel
gtk3-devel
webkit2gtk3-devel
libappindicator3-devel
librsvg-devel
patchelf
EOF
    ;;
  esac
}

ensure_npm_is_linux() {
  if ! have_cmd npm; then
    echo "${ERR} npm not found."
    exit 1
  fi
  local npm_path
  npm_path="$(command -v npm)"
  if is_wsl && [[ "$npm_path" == *".exe" ]]; then
    echo "${ERR} Windows npm inside WSL detected: $npm_path"
    echo "${ERR} Install Linux npm (apt): sudo apt-get install -y nodejs npm"
    exit 1
  fi
}

ensure_toolchain_cmds() {
  for t in cmake ninja "${CC}" "${CXX}"; do
    if ! have_cmd "$t"; then
      echo "${ERR} Required tool missing: $t"
      exit 1
    fi
  done
}

ensure_deps() {
  detect_pm
  if [[ -z "$PM" ]]; then
    echo "${ERR} No supported package manager found (apt/dnf/pacman/zypper)."
    exit 1
  fi

  mapfile -t CPP_PKGS < <(pkgs_cpp_for_pm)
  ensure_pkgs "${CPP_PKGS[@]}"

  if [[ "$TARGET_LC" == "installer" || "$TARGET_LC" == "all" ]]; then
    mapfile -t TAURI_PKGS < <(pkgs_tauri_for_pm)
    ensure_pkgs "${TAURI_PKGS[@]}"
    ensure_npm_is_linux
  fi

  ensure_toolchain_cmds
}

build_neoluma() {
  echo "${INFO} Building neoluma..."
  cmake --build --preset "${BUILD_PRESET}" --target install
  local out=".build/.runtime/${CONFIG}/bin/neoluma"
  [[ -f "$out" ]] && echo "${OK} Built: $out" || true
}

build_payload() {
  echo "${INFO} Building payload..."
  cmake --build --preset "${BUILD_PRESET}" --target payload
}

build_tauri() {
  echo "${INFO} Building Tauri installer..."
  pushd src/Installer >/dev/null
  npm run tauri build
  popd >/dev/null
}

# ---- Run ----
echo "${INFO} Mode: ${MODE}"
echo "${INFO} Target: ${TARGET}"
echo "${INFO} Using toolchain: CC=${CC}, CXX=${CXX} (libc++)"

ensure_deps

echo "${INFO} Configuring..."
cmake --preset "${CFG_PRESET}"

case "$TARGET_LC" in
  neoluma)   build_neoluma ;;
  payload)   build_payload ;;
  installer|all)
    build_payload
    build_tauri
    ;;
  *)
    echo "${ERR} Unknown target: ${TARGET}"
    echo "Valid: neoluma, payload, installer, all"
    exit 1
    ;;
esac

echo "${DONE}"