cmake_minimum_required(VERSION 3.16)
project(flow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

enable_testing()

add_subdirectory(sources)
add_subdirectory(tests)