cmake_minimum_required(VERSION 3.15)
project(poker_gui LANGUAGES CXX)

# 1) Compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-O3 -ffast-math)

# 2) Find dependencies
find_package(FLTK REQUIRED)     
find_package(TBB   REQUIRED)   

# 3) Include paths
include_directories(
  ${FLTK_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
)

# 4) Gather your sources
file(GLOB_RECURSE SOURCES
  gui/gui.cpp
  src/hands/*.cpp
  src/solver/*.cpp
  src/trainer/*.cpp
  src/tree/*.cpp
)

# 5) Define the executable
add_executable(poker_gui ${SOURCES})

# 6) Link against FLTK, TBB and your static libpheval
target_link_libraries(poker_gui
  ${FLTK_LIBRARIES}
  TBB::tbb
  ${CMAKE_SOURCE_DIR}/include/libpheval.a
)

