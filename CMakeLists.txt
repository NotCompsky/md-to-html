cmake_minimum_required(VERSION 3.16)
project(md_to_html)

set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")

add_executable(md_to_html src/main.cpp src/md_to_html.cpp src/inline_functions.cpp)

target_include_directories(md_to_html PRIVATE src)
