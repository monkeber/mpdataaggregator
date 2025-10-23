project(common LANGUAGES CXX)

set(${PROJECT_NAME}_SOURCES
    src/Core.cpp
    src/Utils.cpp
)

set(${PROJECT_NAME}_HEADERS
    include/common/Core.h
    include/common/Utils.h
)

add_library(${PROJECT_NAME} STATIC
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        fmt::fmt
)
