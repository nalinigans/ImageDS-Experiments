#distutils: language = c++
#cython: language_level=3

from __future__ import absolute_import, print_function
from enum import IntEnum

import numpy as np

from libcpp.vector cimport vector
from cython.operator cimport dereference as deref, preincrement as inc
from cython.operator cimport dereference

include "utils.pxi"

class compression_type(IntEnum):
    NONE=compression_t.NONE
    GZIP=compression_t.GZIP
    ZSTD=compression_t.ZSTD
    LZ4=compression_t.LZ4
    BLOSC=compression_t.BLOSC
    BLOSC_LZ4=compression_t.BLOSC_LZ4
    BLOSC_LZ4HC=compression_t.BLOSC_LZ4HC
    BLOSC_SNAPPY=compression_t.BLOSC_SNAPPY
    BLOSC_ZLIB=compression_t.BLOSC_ZLIB
    BLOSC_ZSTD=compression_t.ZSTD
    BLOSC_RLE=compression_t.BLOSC_RLE

cdef attr_type_t to_attr_type(dtype):
    if dtype == np.char:
        return CHAR
    elif dtype == np.int8 or dtype == np.byte:
        return INT8
    elif dtype == np.int16:
        return INT16
    elif dtype == np.int32:
        return INT32
    elif dtype == np.int64:
        return INT64
    elif dtype == np.uint8 or dtype == np.ubyte:
        return UINT8
    elif dtype == np.uint16:
        return UINT16
    elif dtype == np.uint32:
        return UINT32
    elif dtype == np.uint64:
        return UINT64
    elif dtype == np.float32:
        return FLOAT32
    elif dtype == np.float64:
        return FLOAT64
    raise TypeError("Unsupported data type '{0!r}'".format(dtype))

cdef np.dtype to_dtype(attr_type_t attr_type):
    if attr_type == CHAR:
        return np.dtype(np.char)
    elif attr_type == INT8:
        return np.dtype(np.int8)
    elif attr_type == INT16:
        return np.dtype(np.int16)
    elif attr_type == INT32:
        return np.dtype(np.int32)
    elif attr_type == INT64:
        return np.dtype(np.int64)
    elif attr_type == UINT8:
        return np.dtype(np.uint8)
    elif attr_type == UINT16:
        return np.dtype(np.uint16)
    elif attr_type == UINT32:
        return np.dtype(np.uint32)
    elif attr_type == UINT8:
        return np.dtype(np.uint8)
    elif attr_type == FLOAT32:
        return np.dtype(np.float32)
    elif attr_type == FLOAT64:
        return np.dtype(np.float64)
    raise TypeError("Unsupported attribute data type {0}".format(attr_type))

def version():
    version_string = imageds_version()
    return version_string

cdef class _ImageDS:
    cdef ImageDS* _imageds

    def __cinit__(self):
        self._imageds = NULL

    def __init__(self,
                 workspace,
                 overwrite = False,
                 disable_file_locking = False):
        self._imageds = new ImageDS(as_string(workspace),
                                    overwrite,
                                    disable_file_locking)
        if self._imageds is NULL:
            raise MemoryError()

    def array_info(self, path):
        cdef ImageDSArray array
        if self._imageds.array_info(as_string(path), array) != 0:
            raise RuntimeError("Could not get array_info for "+path)

    cdef to_image(self, _ImageDSArray array, vector[void *]buffers, vector[size_t] sizes):
        return self._imageds.to_array(array.get()[0], buffers, sizes)

    cdef from_image(self, _ImageDSArray array, vector[void *]buffers, vector[size_t] sizes):
        return self._imageds.from_array(array.get()[0], buffers, sizes)

cdef _ImageDS _imageds
def setup(workspace):
    global _imageds # necessary
    _imageds = _ImageDS(workspace)

cdef class _ImageDSArray(object):
    cdef ImageDSArray* _array

    def __cinit__(self):
        self._array = NULL

    def __init__(self, path):
        self._array = new ImageDSArray(as_string(path))

    def __setitem__(self, key, value):
        if self._array.attributes().size() != 1:
            raise RuntimeError("Only ImageDS arrays with 1 attribute is supported for now")
        if not isinstance(key, slice):
            raise TypeError("Unsupported subscriptable key type '{0}'".format(type(key)))
        if not isinstance(value, np.ndarray):
            raise TypeError("Unsupported subscriptable value type '{0}'".format(type(value)))
        if key.start != None or key.stop != None or key.step != None:
            raise RuntimeError("Only writing the entire array with all dimensions/attributes supported for now")
        assert(value.ndim == self._array.dimensions().size())
        assert(value.flags.c_contiguous)
        cdef vector[void *] buffers
        cdef vector[size_t] buffer_sizes
        buffers.push_back(np.PyArray_DATA(value))
        buffer_sizes.push_back(value.nbytes)
        cdef int rc = _imageds.to_image(self, buffers, buffer_sizes)
        assert(rc == 0)

    def __array__(self, dtype=None):
        a = self[...]
        if dtype and a.dtype != dtype:
            a = a.astype(dtype)
        return a

    def __repr__(self):
        return self.__array__().__repr__()

    def __getitem__(self, object key):
        if self._array.attributes().size() != 1:
            raise RuntimeError("Only ImageDS arrays with 1 attribute is supported for now")
        if not isinstance(key, slice):
            raise TypeError("Unsupported subscriptable key type '{0}'".format(type(key)))
        if key.start != None or key.stop != None or key.step != None:
            raise RuntimeError("Only reading the entire array with all dimensions/attributes supported for now")
        nbytes = 0
        dim_list = []
        for i in range(self._array.dimensions().size()):
            dim_list.append(deref(self._array.dimensions().data()[i]).end()
                            -deref(self._array.dimensions().data()[i]).start() + 1)
        np_array = np.empty(tuple(dim_list), dtype=to_dtype(deref(self._array.attributes().data()[0]).type()), order='C')
        cdef vector[void *] buffers
        cdef vector[size_t] buffer_sizes
        buffers.push_back(np.PyArray_DATA(np_array))
        buffer_sizes.push_back(np_array.nbytes)
        cdef int rc = _imageds.from_image(self, buffers, buffer_sizes)
        assert(rc == 0)
        return np_array

    cdef ImageDSArray *get(self):
        return self._array

    def add_dimension(self, name, start, end, tile_extent):
        self._array.add_dimension(name, start, end, tile_extent)

    def add_attribute(self, name, attr_type, compression=NONE, compression_level=0):
        self._array.add_attribute(name, attr_type, compression, compression_level)

class Py_ImageDSDimension:
    def __init__(self, name, start, end, tile_extent):
        self._name = name
        self._start = start
        self._end = end
        self._tile_extent = tile_extent

def array_dimension(name, start, end, tile_extent):
    return Py_ImageDSDimension(name, start, end, tile_extent)

class Py_ImageDSAttribute:
    def __init__(self, name, attr_type_t attr_type, compression_t compression, compression_level):
        self._name = name
        self._attr_type = attr_type
        self._compression = compression
        self._compression_level = compression_level

def cell_attribute(name, dtype, compression_t compression=NONE, compression_level=0):
    return Py_ImageDSAttribute(name, to_attr_type(dtype), compression, compression_level)

def define_array(path, dimensions, attributes):
    cdef imageds_array = _ImageDSArray(as_string(path))
    if len(dimensions) == 0:
        raise RuntimeError("Specify at least one dimension while defining array")
    if len(attributes) == 0:
        raise RuntimeError("Specify at least one attribute while defining array")
    for dimension in dimensions:
        imageds_array.add_dimension(as_string(dimension._name),
                                    dimension._start,
                                    dimension._end,
                                    dimension._tile_extent)
    for attribute in attributes:
        imageds_array.add_attribute(as_string(attribute._name),
                                    attribute._attr_type,
                                    attribute._compression,
                                    attribute._compression_level)
    return imageds_array

