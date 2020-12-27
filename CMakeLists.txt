cmake_minimum_required(VERSION 3.19)
project(tvtorrent CXX)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(LIBTORRENT REQUIRED libtorrent libtorrent-rasterbar)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

add_subdirectory(src)