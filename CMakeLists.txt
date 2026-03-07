cmake_minimum_required(VERSION 3.25)
project(YoutubeVideoDownloader C)

set(CMAKE_C_STANDARD 23)

add_executable(YoutubeVideoDownloader main.c tinyfiledialogs.c)

find_package(raylib CONFIG REQUIRED)
target_link_libraries(YoutubeVideoDownloader PRIVATE raylib)