cmake_minimum_required(VERSION 3.0)

project(muQuinet
  VERSION 0.1.0
  # muQuinetd is a C++ project
  # posix-sockets is a pure C project
  LANGUAGES C CXX)

# Linux only
if(NOT CMAKE_SYSTEM_NAME STREQUAL Linux)
  message(FATAL_ERROR
    "muQuinet is supposed to build and run on **LINUX ONLY**")
endif(NOT CMAKE_SYSTEM_NAME STREQUAL Linux)

# Not intended to be build as a subproject
if(NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  message(FATAL_ERROR
    "muQuinet can't be build as a subproject")
endif(NOT CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)

############################################################
# CMake configurations

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${PROJECT_SOURCE_DIR}/cmake"
  "${PROJECT_SOURCE_DIR}/cmake/modules")

############################################################
# Build Specification
#
# Standard conformance & compiler flags
#  - Debug/Release build types
#  - static/shared library building
#  - Fundamental/Essential Compiler flags
#  - AddressSanitizer, ThreadSanitizer

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release
    CACHE STRING "Build Type" FORCE)
endif()

# Shared libraries
option(BUILD_SHARED_LIBS "Whether to build shared libraries (not affecting lib<posix-sockets> which must be build as shared library)"
  ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

set(muQuinet_C_CXX_FLAGS
  "-Wall \
  -Wextra"
  )
set(muQuinet_C_CXX_FLAGS_DEBUG
  "-ggdb"
  )

option(USE_ADDRESS_SANITIZER "Whether to use Google Address Sanitizer in Debug mode" OFF)
option(USE_THREAD_SANITIZER "Whether to use Google Thread Sanitizer in Debug Mode" OFF)

# Sanitizer in Debug mode only
if(USE_ADDRESS_SANITIZER)
  set(muQuinet_C_CXX_FLAGS_DEBUG
    "${muQuinet_C_CXX_FLAGS_DEBUG} \
    -fsanitize=address \
    -fno-omit-frame-pointer \
    -fno-optimize-sibling-calls \
    -fno-common"
    )
endif()
if(USE_THREAD_SANITIZER)
  set(muQuinet_C_CXX_FLAGS_DEBUG
    "${muQuinet_C_CXX_FLAGS_DEBUG} \
    -fsanitize=thread"
    )
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${muQuinet_C_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${muQuinet_C_CXX_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG
  "${CMAKE_C_FLAGS_DEBUG} ${muQuinet_C_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} ${muQuinet_C_CXX_FLAGS_DEBUG}")

############################################################
# Build Output Specification
#
# Make a package ${PROJECT_BINARY_DIR}/muQuinet-<version> at build time.
#  - package holds {bin,lib,etc} structure
#  - You can just copy the muQuinet-<version> directory
#    to somewhere else to run muQuinet, e.g. /opt

set(muQuinet_OUTPUT_DIR "${PROJECT_BINARY_DIR}/muquinet-${PROJECT_VERSION}")
execute_process(
  COMMAND cmake -E remove_directory "${muQuinet_OUTPUT_DIR}"
  )

set(muQuinet_RUNTIME_OUTPUT_DIR ${muQuinet_OUTPUT_DIR}/bin)
set(muQuinet_LIBRARY_OUTPUT_DIR ${muQuinet_OUTPUT_DIR}/lib)
set(muQuinet_ETC_OUTPUT_DIR ${muQuinet_OUTPUT_DIR}/etc)
set(muQuinet_LOG_OUTPUT_DIR ${muQuinet_OUTPUT_DIR}/log)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${muQuinet_RUNTIME_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${muQuinet_LIBRARY_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${muQuinet_LIBRARY_OUTPUT_DIR})

set(CMAKE_BUILD_RPATH "\$ORIGIN/../lib")

############################################################
# Dependency

# https://cmake.org/cmake/help/v3.10/module/FindBoost.html
find_package(Boost
  REQUIRED COMPONENTS
    program_options
    log
    thread
  )
add_definitions(-DBOOST_ALL_DYN_LINK)

# https://cmake.org/cmake/help/v3.11/manual/cmake-qt.7.html
# find_package(Qt5
#   REQUIRED COMPONENTS
#   Core)

# https://cmake.org/cmake/help/v3.11/module/FindProtobuf.html
find_package(Protobuf
  REQUIRED)

############################################################
# Config & Copy file

execute_process(
  COMMAND cmake -E make_directory
      ${muQuinet_RUNTIME_OUTPUT_DIR}
      ${muQuinet_LIBRARY_OUTPUT_DIR}
      ${muQuinet_ETC_OUTPUT_DIR}
      ${muQuinet_LOG_OUTPUT_DIR}
  )
    
execute_process(    
  COMMAND cmake -E copy
      ${PROJECT_SOURCE_DIR}/release/bin/muquinet-wrapper.sh
      ${muQuinet_RUNTIME_OUTPUT_DIR}/muquinet
  COMMAND cmake -E copy_directory
      ${PROJECT_SOURCE_DIR}/release/etc/muquinet
      ${muQuinet_ETC_OUTPUT_DIR}
  )

configure_file(
  ${PROJECT_SOURCE_DIR}/release/etc/systemd/system/muquinetd.service.in
  ${muQuinet_ETC_OUTPUT_DIR}/systemd/system/muquinetd.service
  @ONLY
  )

configure_file(
  ${PROJECT_SOURCE_DIR}/src/muquinet.h.in
  ${PROJECT_BINARY_DIR}/muquinet.h
  @ONLY
  )

# generate protobuf C/C++ code
add_subdirectory(src/rpc)

############################################################
# Subdirectories

include_directories(${PROJECT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(src)

add_subdirectory(third-party)
add_subdirectory(src/muquinetd)
add_subdirectory(src/interceptor)

add_subdirectory(tests)
# add_subdirectory(apps)

############################################################
# Install Configurations

# RPATH

list(FIND
  CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES
  "${CMAKE_INSTALL_PREFIX}/lib"
  isSystemDir)
if ("${isSystemDir}" STREQUAL "-1")
    list(APPEND CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
endif ()
