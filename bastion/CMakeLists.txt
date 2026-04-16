cmake_minimum_required(VERSION 3.16)
project(PhantomBrowser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Security-hardened compile flags ──────────────────────────────────────────
add_compile_options(
    -Wall -Wextra -Wpedantic -Wformat -Wformat-security -Wnull-dereference
    -fstack-protector-strong
    -fstack-clash-protection
    -fPIE
    -D_FORTIFY_SOURCE=2
    -fcf-protection=full
    -Wno-unused-parameter
    -O2
)

# ─── Security-hardened linker flags ───────────────────────────────────────────
add_link_options(
    -pie
    -Wl,-z,relro
    -Wl,-z,now
    -Wl,-z,noexecstack
    -Wl,-z,separate-code
)

# ─── Dependencies ─────────────────────────────────────────────────────────────
find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK    REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
pkg_check_modules(SOUP   REQUIRED IMPORTED_TARGET libsoup-3.0)
pkg_check_modules(GLIB   REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(PANGO  REQUIRED IMPORTED_TARGET pango)

# gio is included inside glib — no separate package needed
find_library(GIO_LIB gio-2.0)

# ─── Sources ──────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/browser_window.cpp
    src/privacy_engine.cpp
    src/proxy_chain.cpp
    src/content_filter.cpp
)

add_executable(phantom ${SOURCES})

target_include_directories(phantom PRIVATE src/)

target_link_libraries(phantom PRIVATE
    PkgConfig::GTK
    PkgConfig::WEBKIT
    PkgConfig::SOUP
    PkgConfig::GLIB
    PkgConfig::PANGO
)

# ─── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS phantom DESTINATION ${CMAKE_INSTALL_BINDIR})

message(STATUS "──────────────────────────────────────────────────")
message(STATUS "  Phantom Browser")
message(STATUS "  GTK     : ${GTK_VERSION}")
message(STATUS "  WebKit  : ${WEBKIT_VERSION}")
message(STATUS "  libsoup : ${SOUP_VERSION}")
message(STATUS "──────────────────────────────────────────────────")
