cmake_minimum_required(VERSION 3.15)
project(shark LANGUAGES CXX)

# 1) Compiler settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (MSVC)
  # /O2 fine
else()
  add_compile_options(-O3 -ffast-math)
endif()

# 2) Find external deps (dynamic on Windows, static only for FLTK if you choose)
find_package(FLTK REQUIRED)
find_package(TBB   REQUIRED)

# 3) PokerHandEvaluator
set(BUILD_CARD5    OFF CACHE BOOL "" FORCE)
...
add_subdirectory(include/PokerHandEvaluator/cpp pheval_build)

# 4) Sources & executable
file(GLOB_RECURSE POKER_GUI_SRCS
  gui/gui.cpp
  src/hands/*.cpp
  src/solver/*.cpp
  src/trainer/*.cpp
  src/tree/*.cpp
)
add_executable(shark ${POKER_GUI_SRCS})

# 5) Includes
target_include_directories(shark PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
  ${FLTK_INCLUDE_DIR}
)

# 6) Link (dynamic TBB on all platforms)
target_link_libraries(shark PRIVATE
  ${FLTK_LIBRARIES}
  TBB::tbb
  pheval
)
