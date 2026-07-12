cmake_minimum_required(VERSION 3.16)
project(zen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer Homebrew/dylib SDL2 over any stale SDL2.framework in /Library/Frameworks —
# browser-downloaded frameworks are quarantined and Gatekeeper refuses to load them.
set(CMAKE_FIND_FRAMEWORK LAST)

add_executable(zen
    src/main.cpp
    src/chronicle.cpp
    src/garden.cpp
    src/garden_generator.cpp
    src/life_system.cpp
    src/texture_manager.cpp
    src/water_system.cpp
    src/weather_system.cpp
    src/world_renderer.cpp
    src/cloud_manager.cpp
)

target_include_directories(zen PRIVATE include)

if(MSVC)
    target_compile_definitions(zen PRIVATE _USE_MATH_DEFINES NOMINMAX)
endif()

# ---- SDL2 ----------------------------------------------------------------
# Prefer the CMake config packages (vcpkg, homebrew, most modern distros),
# fall back to pkg-config (older linux setups).
find_package(SDL2 CONFIG QUIET)
if(TARGET SDL2::SDL2)
    target_link_libraries(zen PRIVATE SDL2::SDL2)
    if(TARGET SDL2::SDL2main)
        target_link_libraries(zen PRIVATE SDL2::SDL2main)
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_PKG REQUIRED IMPORTED_TARGET sdl2)
    target_link_libraries(zen PRIVATE PkgConfig::SDL2_PKG)
endif()

# ---- SDL2_image ----------------------------------------------------------
find_package(SDL2_image CONFIG QUIET)
if(TARGET SDL2_image::SDL2_image)
    target_link_libraries(zen PRIVATE SDL2_image::SDL2_image)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_IMAGE_PKG REQUIRED IMPORTED_TARGET SDL2_image)
    target_link_libraries(zen PRIVATE PkgConfig::SDL2_IMAGE_PKG)
endif()

# ---- Assets --------------------------------------------------------------
# Copy assets next to the binary so paths resolve from the exe dir on all OSes.
add_custom_command(TARGET zen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:zen>/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:zen>/world_info
)

# ---- Headless conservation test -------------------------------------------
add_executable(conservation_test
    test/conservation_test.cpp
    src/water_system.cpp
    src/weather_system.cpp
    src/cloud_manager.cpp
)
target_include_directories(conservation_test PRIVATE include)
if(TARGET SDL2::SDL2)
    target_link_libraries(conservation_test PRIVATE SDL2::SDL2)
else()
    target_link_libraries(conservation_test PRIVATE PkgConfig::SDL2_PKG)
endif()
if(MSVC)
    target_compile_definitions(conservation_test PRIVATE _USE_MATH_DEFINES NOMINMAX)
endif()
