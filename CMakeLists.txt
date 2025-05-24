cmake_minimum_required(VERSION 3.15)
project(shark LANGUAGES CXX)

# ——————————————————————————————————————————————
# 1) Compiler settings
# ——————————————————————————————————————————————
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O3 -ffast-math)

# ——————————————————————————————————————————————
# 2) Find system libraries
# ——————————————————————————————————————————————
find_package(FLTK REQUIRED)
find_package(TBB   REQUIRED)

# ——————————————————————————————————————————————
# 3) Disable all the extra PHE targets
# ——————————————————————————————————————————————
# note: these must appear *before* the add_subdirectory() call
set(BUILD_CARD5    OFF CACHE BOOL "disable 5‑card lib"    FORCE)
set(BUILD_CARD6    OFF CACHE BOOL "disable 6‑card lib"    FORCE)
set(BUILD_CARD7    OFF CACHE BOOL "disable 7‑card lib"    FORCE)
set(BUILD_PLO4     OFF CACHE BOOL "disable plo4 lib"      FORCE)
set(BUILD_PLO5     OFF CACHE BOOL "disable plo5 lib"      FORCE)
set(BUILD_PLO6     OFF CACHE BOOL "disable plo6 lib"      FORCE)
set(BUILD_TESTS    OFF CACHE BOOL "disable tests"         FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "disable examples"      FORCE)

# ——————————————————————————————————————————————
# 4) Build the PokerHandEvaluator sub‑project
# ——————————————————————————————————————————————
add_subdirectory(include/PokerHandEvaluator/cpp pheval_build)
# this will only build the `pheval` target (libpheval.a)

# ——————————————————————————————————————————————
# 5) Collect your own sources & executable
# ——————————————————————————————————————————————
file(GLOB_RECURSE POKER_GUI_SRCS
  gui/gui.cpp
  src/hands/*.cpp
  src/solver/*.cpp
  src/trainer/*.cpp
  src/tree/*.cpp
)

add_executable(shark ${POKER_GUI_SRCS})

# ——————————————————————————————————————————————
# 6) Include paths
# ——————————————————————————————————————————————
target_include_directories(shark PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
  ${FLTK_INCLUDE_DIR}
)

# ——————————————————————————————————————————————
# 7) Link libraries
# ——————————————————————————————————————————————
target_link_libraries(shark PRIVATE
  ${FLTK_LIBRARIES}  # or FLTK::FLTK if your FindFLTK provides a target
  TBB::tbb
  pheval             # the static lib from the sub‑project
)

