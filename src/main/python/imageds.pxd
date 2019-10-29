# distutils: language = c++
# cython: language_level=3

from libcpp.pair cimport pair
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.functional cimport function
from libc.stdint cimport (int32_t, uint32_t, int64_t, uint64_t, uintptr_t, INT64_MAX)
from cpython cimport (PyObject, PyList_New)

cdef extern from "imageds.h":
  cdef string imageds_version()
