#
# CMakeLists.txt
#
# The MIT License
#
# Copyright (c) 2019 Nalini Ganapati
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

project(ImageDS)

cmake_minimum_required(VERSION 3.3)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

if(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
  set(CMAKE_FIND_FRAMEWORK LAST)
endif()

# Update Submodules
execute_process(COMMAND git submodule update --recursive --init
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		RESULT_VARIABLE submodule_update_exit_code)
if(NOT(submodule_update_exit_code EQUAL 0))
  message(FATAL_ERROR "Failure to recursively update git submodules")
endif()

# Get git latest commit hash to use with ImageDS Version
execute_process(COMMAND git log -1 --pretty=format:%h
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE GIT_COMMIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Build Options
set(MAJOR 0)
set(MINOR 1)
set(PATCH 1)
set(IMAGEDS_VERSION "${MAJOR}.${MINOR}.${PATCH}-${GIT_COMMIT_HASH}" CACHE STRING "ImageDS full version string")
set(IMAGEDS_VERBOSE True CACHE BOOL "Prints errors with verbosity")
set(IMAGEDS_TRACE False CACHE BOOL "Trace functionality, only available in Debug mode")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(TILEDB_SOURCE_DIR "${CMAKE_SOURCE_DIR}/dependencies/TileDB" CACHE PATH "Path to TileDB source directory")
set(DISABLE_MPI True CACHE BOOL "Disable use of any MPI compiler/libraries")
set(DISABLE_OPENMP False CACHE BOOL "Disable OpenMP")
set(BUILD_DISTRIBUTABLE_LIBRARY False CACHE BOOL "Build ImageDS library with minimal runtime dependencies")

# Compile Options
set(CMAKE_CXX_STANDARD 11) # C++11 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # GNU extensions is OFF
set(CMAKE_C_VISIBILITY_PRESET hidden) #-fvisibility=hidden
set(CMAKE_CXX_VISIBILITY_PRESET hidden) #-fvisibility=hidden
add_compile_options(-Wall -Wextra)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")  
  add_compile_options(-DDEBUG -O0  -g3 -ggdb3 -gdwarf-3)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-DNDEBUG -O3)
elseif (CMAKE_BUILD_TYPE STREQUAL "Coverage")
  add_compile_options(-DDEBUG -g3 -gdwarf-3 --coverage)
endif()
if (NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-literal-suffix")
endif()

# Add definitions
if(IMAGEDS_TRACE)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
     add_definitions(-DIMAGEDB_TRACE)
     message(STATUS "The ImageDS library is compiled with trace.")
  else()
     message(STATUS "ImageDS in release mode cannot have trace. CMAKE_BUILD_TYPE=" ${CMAKE_BUILD_TYPE})
  endif()
endif()
add_definitions(-DIMAGEDS_VERSION=\"${IMAGEDS_VERSION}\")

# External packages required by TileDB
find_package(ZLIB REQUIRED)
find_package(libuuid REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(JNI REQUIRED)

if (NOT DISABLE_MPI)
  find_package(MPI)
  if (MPI_FOUND)
    set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
  endif()
endif()

if (NOT DISABLE_OPENMP AND NOT APPLE)
  find_package(OpenMP)
  if (OpenMP_FOUND)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif()
endif()

# Build TileDB
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW) # Honor visibility properties for all targets
find_package(TileDB REQUIRED)

set(IMAGEDS_DEPENDENCIES
  ${OPENSSL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${LIBUUID_LIBRARY}
  ${JAVA_JVM_LIBRARY}
)

# Build ImageDS 
enable_testing()
add_subdirectory(src)

#Uninstall ImageDS
add_custom_target(
   uninstall
   COMMAND echo "-- Uninstalling ImageDS from ${CMAKE_INSTALL_PREFIX}..."
   COMMAND < install_manifest.txt xargs -I % sh -c 'echo -- Uninstalling % && rm -f %' %
   COMMAND echo "-- ImageDS is uninstalled!"
)
