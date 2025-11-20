cmake_minimum_required(VERSION 3.15)

project(emuNES VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB CORE_SOURCES "src/*.cpp")
file(GLOB CORE_HEADERS "include/*.h")

list(REMOVE_ITEM CORE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/tests.cpp"
)

add_library(core_logic STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(core_logic PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

include_directories(/opt/homebrew/include/SDL2)

find_package(SDL2 REQUIRED)

find_package(SDL2_mixer REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(emuNES PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    core_logic
    ${SDL2_LIBRARIES}
    /opt/homebrew/lib/libSDL2_mixer.dylib
)
