cmake_minimum_required(VERSION 3.15)
project(shark LANGUAGES CXX)

#
# 0) STATIC‑LINK ON MinGW64 (MSYS2)
#
if (WIN32 AND MINGW)
  # 0.1) Prefer .a (static) over .dll.a / .lib
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")

  # 0.2) Disable building/sharing any DLLs
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Disable shared libraries" FORCE)

  # 0.3) Ask the linker to pull in CRT/Runtimes statically
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

  # 0.4) Tell FLTK headers we’re linking the static flavor
  add_compile_definitions(FLTK_STATIC)

  # 0.5) Find the static TBB archive in MSYS2’s lib dir
  find_library(TBB_STATIC_LIB
    NAMES tbb
    PATHS "$ENV{MSYSTEM_PREFIX}/lib"
    NO_DEFAULT_PATH
    DOC "Static TBB library (libtbb.a) in MSYS2"
  )
  if (NOT TBB_STATIC_LIB)
    message(FATAL_ERROR
      "Could not find libtbb.a in $ENV{MSYSTEM_PREFIX}/lib. "
      "Please install mingw-w64-x86_64-tbb so that libtbb.a is available."
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
  # /O2 is fine
else()
  add_compile_options(-O3 -ffast-math)
endif()

#
# 2) Find external deps
#
find_package(FLTK REQUIRED)        # picks up .a now because BUILD_SHARED_LIBS=OFF
if (NOT (WIN32 AND MINGW))
  find_package(TBB REQUIRED)       # on other platforms use the imported target
endif()

#
# 3) PokerHandEvaluator subproject
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
  target_link_libraries(shark PRIVATE
    ${FLTK_LIBRARIES}     # static FLTK .a files
    ${TBB_STATIC_LIB}     # our manually found libtbb.a
    pheval
  )
else()
  target_link_libraries(shark PRIVATE
    ${FLTK_LIBRARIES}
    TBB::tbb
    pheval
  )
endif()
