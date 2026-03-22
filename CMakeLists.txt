cmake_minimum_required(VERSION 3.15)

project(emuNES VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB CORE_HEADERS CONFIGURE_DEPENDS "include/*.h")

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

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE core_logic)

if (TARGET SDL2::SDL2)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)
elseif (TARGET SDL2::SDL2-static)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2-static)
elseif (DEFINED SDL2_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
    if (DEFINED SDL2_INCLUDE_DIRS)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
    elseif (DEFINED SDL2_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIR})
    endif ()
else ()
    message(FATAL_ERROR "SDL2 was found but no usable SDL2 target or library variables are available.")
endif ()

if (TARGET SDL2_mixer::SDL2_mixer)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer::SDL2_mixer)
elseif (TARGET SDL2_mixer::SDL2_mixer-static)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer::SDL2_mixer-static)
elseif (DEFINED SDL2_MIXER_LIBRARIES)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_LIBRARIES})
    if (DEFINED SDL2_MIXER_INCLUDE_DIRS)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_INCLUDE_DIRS})
    elseif (DEFINED SDL2_MIXER_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_MIXER_INCLUDE_DIR})
    endif ()
else ()
    message(FATAL_ERROR "SDL2_mixer was found but no usable SDL2_mixer target or library variables are available.")
endif ()
