project(consumer)

set(${PROJECT_NAME}_SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME}
    ${${PROJECT_NAME}_SOURCES}
)
