#
# src/test/CMakeLists.txt
#

# The MIT License
#
# Copyright (c) 2019 Omics Data Automation, Inc.
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

add_subdirectory(cpp)

#find_package(PythonInterp 3.6 REQUIRED)

#add_test(NAME python_tests
#         COMMAND ${PYTHON_EXECUTABLE} imageds_test.py
#         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python)
#set_tests_properties(python_tests PROPERTIES
#                     ENVIRONMENT PYTHONPATH=$ENV{PYTHONPATH}:${CMAKE_SOURCE_DIR}/src/main/python)

#add_test(NAME python_zarr_tests
#         COMMAND ${PYTHON_EXECUTABLE} unit_test.py
#         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python/zarr)
#set_tests_properties(python_zarr_tests PROPERTIES
#                     ENVIRONMENT PYTHONPATH=$ENV{PYTHONPATH}:${CMAKE_SOURCE_DIR}/src/main/python/zarr)
