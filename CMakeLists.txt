cmake_minimum_required(VERSION 4.0)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")

# ---- Project Setup ----
project(neoluma LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

message(STATUS "[Neoluma] Using C++ version: ${CMAKE_CXX_STANDARD}")
message(STATUS "[Neoluma] C++ extensions are ${CMAKE_CXX_EXTENSIONS}")

# Options
option(NEOLUMA_PORTABLE "Portable layout (resources near executable)")

# Set up LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "[Neoluma] Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "[Neoluma] Using LLVMConfig.cmake in: ${LLVM_DIR}")

# ---- Sources ----
file(GLOB_RECURSE CORE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Core/*.hpp
)
file(GLOB_RECURSE CLI_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CLI/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CLI/*.hpp
)
file(GLOB_RECURSE LIB_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libraries/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Libraries/*.hpp
)

add_library(NeolumaCore ${CORE_FILES})
add_library(NeolumaCLI ${CLI_FILES})
add_library(NeolumaLibs ${LIB_FILES})

set(NEOLUMA_ICON_RC "")
if(WIN32)
    set(NEOLUMA_ICON_RC "${CMAKE_SOURCE_DIR}/resources/app.rc")
endif()

add_executable(neoluma
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HelperFunctions.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HelperFunctions.cpp
    ${NEOLUMA_ICON_RC}
)

if(APPLE)
    set_target_properties(neoluma PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE "app.icns"
    )

    set(APP_ICON_MACOS "${CMAKE_SOURCE_DIR}/resources/app.icns")
    set_source_files_properties(${APP_ICON_MACOS} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    target_sources(neoluma PRIVATE ${APP_ICON_MACOS})
endif()


# ---- Include dirs (target-based) ----
target_include_directories(NeolumaCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(NeolumaCLI  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(NeolumaLibs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(neoluma PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# LLVM includes/defs
target_include_directories(neoluma PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(neoluma PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(NeolumaCore PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(NeolumaCore PRIVATE ${LLVM_DEFINITIONS})

# ---- LLVM libs ----
llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
)

target_link_libraries(neoluma PRIVATE
    ${LLVM_LIBS}
    NeolumaCLI
    NeolumaCore
    NeolumaLibs
)

# ---- Output layout for executable ----
set(OutputRoot "${CMAKE_CURRENT_SOURCE_DIR}/.build")

# config name: Debug/Release/etc
set_target_properties(neoluma PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OutputRoot}/.executables/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${OutputRoot}/.libs/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${OutputRoot}/.libs/$<CONFIG>"
)

# shuts up cmake about having extensions
target_compile_options(NeolumaCLI PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wno-c23-extensions>
)

# ---- Installation rules ----
include(GNUInstallDirs)

install(TARGETS neoluma
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/Localization/ DESTINATION ${CMAKE_INSTALL_DATADIR}/locales)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/IntrinsicModules/ DESTINATION ${CMAKE_INSTALL_DATADIR}/modules)

# ---- Tests implementation ----
#set(NEOLUMA_TEST_RUNNER "${CMAKE_SOURCE_DIR}/tests/runner/testrunner.py")

#add_custom_target(frontend_tests
#    COMMAND "${NEOLUMA_POWERSHELL_EXECUTABLE}"
#        -ExecutionPolicy Bypass
#        -File "${NEOLUMA_TEST_RUNNER}"
#        -ExePath "$<TARGET_FILE:neoluma>"
#    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#    DEPENDS neoluma
#    COMMENT "Running frontend regression tests..."
#    VERBATIM
#)

# ---- Payload creation. ----
# payload is bundled into installer resources later on.
# make payload dirs unique per platform+arch+config to avoid overwriting when switching OS/builds
set(NEOLUMA_PAYLOAD_ROOT "${CMAKE_SOURCE_DIR}/.build/payload/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/$<CONFIG>")
set(NEOLUMA_STAGE_DIR   "${NEOLUMA_PAYLOAD_ROOT}/stage")
set(NEOLUMA_PAYLOAD_ZIP "${NEOLUMA_PAYLOAD_ROOT}/payload.zip")

# multi-config generators require --config. single-config i use (ninja) doesn't.
set(NEOLUMA_INSTALL_CONFIG_ARG "")
if(CMAKE_CONFIGURATION_TYPES)
    set(NEOLUMA_INSTALL_CONFIG_ARG --config $<CONFIG>)
endif()

add_custom_command(
    OUTPUT "${NEOLUMA_PAYLOAD_ZIP}"

    DEPENDS neoluma

    # clean stage folder
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${NEOLUMA_STAGE_DIR}"

    # create payload root
    COMMAND ${CMAKE_COMMAND} -E make_directory "${NEOLUMA_PAYLOAD_ROOT}"

    # install to stage/ folder
    COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}"
    --prefix "${NEOLUMA_STAGE_DIR}"
    ${NEOLUMA_INSTALL_CONFIG_ARG}

    # remove old zip
    COMMAND ${CMAKE_COMMAND} -E rm -f "${NEOLUMA_PAYLOAD_ZIP}"

    # create zip from stage contents
    COMMAND ${CMAKE_COMMAND} -E chdir "${NEOLUMA_STAGE_DIR}"
        ${CMAKE_COMMAND} -E tar "cf" "${NEOLUMA_PAYLOAD_ZIP}" --format=zip -- .

    COMMENT "[Neoluma] Creating payload.zip for ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR} ($<CONFIG>)..."
    VERBATIM
)

add_custom_target(payload DEPENDS "${NEOLUMA_PAYLOAD_ZIP}")
