#
# src/CMakeLists.txt
#
# The MIT License
#
# Copyright (c) 2018 Omics Data Automation, Inc.
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

include(GenerateExportHeader)

# Honor visibility properties for all target types.
cmake_policy(SET CMP0063 NEW)

set(IMAGEDS_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/main")

set(IMAGEDS_API
  ${IMAGEDS_MAIN}/cpp/imageds.h
)

set(IMAGEDS_SOURCES
  ${IMAGEDS_MAIN}/cpp/imageds.cc
)

# Use PIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
add_library(imageds_objs OBJECT ${IMAGEDS_SOURCES})

# set(EXPORTS_HEADER ${CMAKE_BINARY_DIR}/src/imageds_export.h)
# generate_export_header(imageds_objs EXPORT_FILE_NAME ${EXPORTS_HEADER})

target_include_directories(imageds_objs
  PUBLIC ${CMAKE_BINARY_DIR}/src
  PRIVATE ${IMAGEDS_MAIN} ${TILEDB_INCLUDE_DIR}
)

add_definitions(-DIMAGEDS_VERBOSE)

set(MAJOR 0)
set(MINOR 1)
set(PATCH 1)
set(IMAGEDS_VERSION ${MAJOR}.${MINOR}.${PATCH})

message(STATUS "IMAGEDS_DEPENDENCIES=" ${IMAGEDS_DEPENDENCIES})

# Create static library
add_library(imageds_static STATIC $<TARGET_OBJECTS:imageds_objs>)
set_target_properties(imageds_static PROPERTIES OUTPUT_NAME "imageds" VERSION ${IMAGEDS_VERSION})
target_link_libraries(imageds_static
  tiledb_static
  ${IMAGEDS_DEPENDENCIES}
  pthread
  ${CMAKE_DL_LIBS} # add -ldl
) 

# Create shared library
add_library(imageds_shared SHARED $<TARGET_OBJECTS:imageds_objs>)
set_target_properties(imageds_shared PROPERTIES OUTPUT_NAME "imageds" VERSION ${IMAGEDS_VERSION} SOVERSION 1)
target_link_libraries(imageds_shared 
  tiledb_static
  ${IMAGEDS_DEPENDENCIES}
  pthread
  ${CMAKE_DL_LIBS} # add -ldl
)

# Install libraries
install(
  TARGETS imageds_static imageds_shared
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(
  FILES ${IMAGEDS_API} ${EXPORTS_HEADER} DESTINATION include
  )

enable_testing()
add_subdirectory(test)
