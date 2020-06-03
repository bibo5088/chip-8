cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(Chip8EmuStandalone
  LANGUAGES CXX
)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(
  NAME Chip8Emu
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
)

find_package(SDL2 CONFIG REQUIRED)

# ---- Create standalone executable ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

add_executable(Chip8EmuStandalone ${sources})

set_target_properties(Chip8EmuStandalone PROPERTIES 
  CXX_STANDARD 17 
  OUTPUT_NAME "Chip8Emu"
)
target_link_libraries(Chip8EmuStandalone PRIVATE Chip8Emu SDL2::SDL2main SDL2::SDL2-static)
