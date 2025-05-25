cmake_minimum_required(VERSION 3.15)
project(shark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
  add_compile_options(-O3 -ffast-math)
endif()

# Optional static linking logic for Unix-like systems
if(APPLE OR UNIX)
  message(STATUS "Enabling static linking for TBB and FLTK")

  set(FLTK_USE_STATIC_LIBS ON)
  add_definitions(-DSTATIC_FLTK)

  # Adjust these if using nonstandard paths
  set(FLTK_STATIC_LIB /opt/homebrew/opt/fltk/lib/libfltk.a)
  set(TBB_STATIC_LIB /opt/homebrew/opt/tbb/lib/libtbb.a)

  if(UNIX AND NOT APPLE)
    set(FLTK_STATIC_LIB /usr/lib/x86_64-linux-gnu/libfltk.a)
    set(TBB_STATIC_LIB /usr/lib/x86_64-linux-gnu/libtbb.a)
  endif()
endif()

# Dependencies
find_package(FLTK REQUIRED)
find_package(TBB REQUIRED)

# PokerHandEvaluator subproject
set(BUILD_CARD5    OFF CACHE BOOL "" FORCE)
set(BUILD_CARD6    OFF CACHE BOOL "" FORCE)
set(BUILD_CARD7    OFF CACHE BOOL "" FORCE)
set(BUILD_PLO4     OFF CACHE BOOL "" FORCE)
set(BUILD_PLO5     OFF CACHE BOOL "" FORCE)
set(BUILD_PLO6     OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(include/PokerHandEvaluator/cpp pheval_build)

file(GLOB_RECURSE POKER_GUI_SRCS
  gui/gui.cpp
  src/hands/*.cpp
  src/solver/*.cpp
  src/trainer/*.cpp
  src/tree/*.cpp
)

add_executable(shark ${POKER_GUI_SRCS})

target_include_directories(shark PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
  ${FLTK_INCLUDE_DIR}
)

if(APPLE OR UNIX)
  target_link_libraries(shark PRIVATE
    ${FLTK_STATIC_LIB}
    ${TBB_STATIC_LIB}
    pheval
  )
else()
  target_link_libraries(shark PRIVATE
    ${FLTK_LIBRARIES}
    TBB::tbb
    pheval
  )
endif()
