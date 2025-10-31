cmake_minimum_required(VERSION 3.15)
project(emuNES VERSION 1.0 LANGUAGES CXX)

file(GLOB CORE_SOURCES "src/*.cpp")

add_executable(emuNES ${CORE_SOURCES})

file(GLOB CORE_INCLUDES "include/*")
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/tests.cpp")
add_library(core_logic STATIC ${CORE_SOURCES})

source_group("core_include" FILES ${CORE_INCLUDES})
target_sources(emuNES PRIVATE ${CORE_INCLUDES})

target_link_libraries(${PROJECT_NAME} PRIVATE core_logic)
