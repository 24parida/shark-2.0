cmake_minimum_required(VERSION 3.15)
project(shark LANGUAGES CXX)

#
# 0) STATIC‑LINK ON MinGW
#
if (WIN32 AND MINGW)
  # 0.1) Search for .a before any .dll.a / .lib
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")

  # 0.2) Disable all shared‐library builds in this project
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared libraries" FORCE)

  # 0.3) Instruct the linker to pull in libgcc, libstdc++ and CRT statically
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

  # 0.4) Tell FLTK headers we’re using a static FLTK runtime
  add_compile_definitions(FLTK_STATIC)

  # 0.5) Locate the static TBB archive (libtbb.a)
  find_library(TBB_STATIC_LIB
    NAMES tbb
    PATHS "$ENV{MSYSTEM_PREFIX}/mingw64/lib"
    NO_DEFAULT_PATH
  )
  if (NOT TBB_STATIC_LIB)
    message(FATAL_ERROR
      "Could not find static TBB (libtbb.a) in $ENV{MSYSTEM_PREFIX}/mingw64/lib. "
      "Make sure you have installed mingw-w64-x86_64-tbb which provides the .a file."
    )
  endif()
endif()

#
# 1) Compiler settings
#
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (MSVC)
  # MSVC: default /O2 is fine
else()
  add_compile_options(-O3 -ffast-math)
endif()

#
# 2) Find external deps
#
find_package(FLTK REQUIRED)       # picks up static .a because BUILD_SHARED_LIBS=OFF
if (NOT (WIN32 AND MINGW))
  find_package(TBB   REQUIRED)    # on non‑MinGW platforms we use the imported target
endif()

#
# 3) Build PokerHandEvaluator subproject
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
if (WIN32 AND MINGW)
  # static‐link FLTK (.a) + our TBB_STATIC_LIB + pheval
  target_link_libraries(shark PRIVATE
    ${FLTK_LIBRARIES}
    ${TBB_STATIC_LIB}
    pheval
  )
else()
  # dynamic FLTK + TBB::tbb imported target + pheval
  target_link_libraries(shark PRIVATE
    ${FLTK_LIBRARIES}
    TBB::tbb
    pheval
  )
endif()
