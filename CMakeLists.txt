cmake_minimum_required(VERSION 3.15)
project(shark LANGUAGES CXX)

#
# 0) FORCE STATIC LINKING WHEN USING MINGW
#
if (WIN32 AND MINGW)
  # 0.1) Look for .a before .lib / .dll.a
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")

  # 0.2) Turn off any shared‐library targets (for add_library() calls)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared libraries" FORCE)

  # 0.3) Ask the linker to pull in the CRTs statically
  #      (so you don't ship MSVCRT.dll / libstdc++.dll, etc.)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

  # 0.4) Tell FLTK headers we’re using the static FLTK build
  add_compile_definitions(FLTK_STATIC)
endif()

#
# 1) Compiler settings
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (MSVC)
  # MSVC’s default /O2 is fine
else()
  add_compile_options(-O3 -ffast-math)
endif()

#
# 2) Find external deps (now will pick up .a libs if available)
#
find_package(FLTK REQUIRED)   # you must have the static FLTK dev package installed
find_package(TBB   REQUIRED)  # likewise for static TBB

#
# 3) Build PokerHandEvaluator subproject statically
#
set(BUILD_CARD5    OFF CACHE BOOL "" FORCE)
set(BUILD_CARD6    OFF CACHE BOOL "" FORCE)
set(BUILD_CARD7    OFF CACHE BOOL "" FORCE)
set(BUILD_PLO4     OFF CACHE BOOL "" FORCE)
set(BUILD_PLO5     OFF CACHE BOOL "" FORCE)
set(BUILD_PLO6     OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(include/PokerHandEvaluator/cpp pheval_build)

#
# 4) Your sources
#
file(GLOB_RECURSE POKER_GUI_SRCS
  gui/gui.cpp
  src/hands/*.cpp
  src/solver/*.cpp
  src/trainer/*.cpp
  src/tree/*.cpp
)
add_executable(shark ${POKER_GUI_SRCS})

#
# 5) Include paths
#
target_include_directories(shark PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
  ${FLTK_INCLUDE_DIR}
)

#
# 6) Link libraries
#
#    FLTK_LIBRARIES should now contain the static libs (libfltk.a, libfltk_images.a, …)
#
target_link_libraries(shark PRIVATE
  ${FLTK_LIBRARIES}
  TBB::tbb
  pheval
)
