@echo off
setlocal EnableExtensions
for /f %%A in ('echo prompt $E ^| cmd') do set "ESC=%%A"

set "MODE=%~1"
if "%MODE%"=="" set "MODE=release"

set "CFG_PRESET=release-ninja"
set "BUILD_PRESET=release"
set "CONFIG=Release"

if /I "%MODE%"=="debug" (
  set "CFG_PRESET=debug-ninja"
  set "BUILD_PRESET=debug"
  set "CONFIG=Debug"
)

echo %ESC%[38;2;232;75;133m[INFO]%ESC%[0m Configure preset: %CFG_PRESET%
echo %ESC%[38;2;232;75;133m[INFO]%ESC%[0m Build preset: %BUILD_PRESET%

echo %ESC%[38;2;232;75;133m[INFO]%ESC%[0m Configuring...
cmake --preset "%CFG_PRESET%"
if errorlevel 1 goto :cfg_fail

goto :build

:build
echo %ESC%[38;2;232;75;133m[INFO]%ESC%[0m Building neoluma...
cmake --build --preset "%BUILD_PRESET%" --target install
if errorlevel 1 goto :build_fail
echo %ESC%[38;2;232;75;133m[INFO]%ESC%[0m Building payload...
cmake --build --preset "%BUILD_PRESET%" --target payload
if errorlevel 1 goto :build_fail
goto :after_build

:after_build
set "EXE=.build\.executables\%CONFIG%\neoluma.exe"
set "RUNTIME=.build\.runtime\%CONFIG%\"
if exist "%EXE%" (
  echo %ESC%[38;2;117;255;135m[SUCCESS]%ESC%[0m Built: %EXE%
) else (
  echo %ESC%[38;2;255;80;80m[ERROR]%ESC%[0m Can't find built exe at: %EXE%
  exit /b 1
)
if exist "%RUNTIME%" (
  echo %ESC%[38;2;117;255;135m[SUCCESS]%ESC%[0m Runtime built at: %RUNTIME%
) else (
  echo %ESC%[38;2;255;80;80m[ERROR]%ESC%[0m Can't find built runtime at: %RUNTIME%
  exit /b 1
)
goto :done

:cfg_fail
echo %ESC%[38;2;255;80;80m[ERROR]%ESC%[0m CMake configure failed.
exit /b 1

:build_fail
echo %ESC%[38;2;255;80;80m[ERROR]%ESC%[0m Build failed.
exit /b 1

:done
echo %ESC%[38;2;117;255;135m[DONE]%ESC%[0m
exit /b 0