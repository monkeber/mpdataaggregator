cmake_minimum_required(VERSION 3.10)

project(mpdataaggregator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(extern/fmt)
add_subdirectory(src/common)
add_subdirectory(src/consumer)
add_subdirectory(src/producer)
