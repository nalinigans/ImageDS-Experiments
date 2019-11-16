#distutils: language = c++
#cython: language_level=3

from __future__ import absolute_import, print_function
from enum import IntEnum

include "utils.pxi"

class attr_type(IntEnum):
    UCHAR = attr_type_t.UCHAR
    INT_8 = attr_type_t.INT_8
    INT_32 = attr_type_t.INT_32
    INT_64 = attr_type_t.INT_64

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

    def __getitem__(self, _ImageDSArray imageds_array):
        print("Nalini-getitem")

    def __setitem__(self, _ImageDSArray imageds_array, buffers):
        print("Nalini-setitem")
       
    def array_info(self, path):
        cdef ImageDSArray array
        if self._imageds.array_info(as_string(path), array) != 0:
            raise RuntimeError("Could not get array_info for"+path)

    def to_image(self, _ImageDSArray array, buffers):
        return 0

    def from_image(self, path):
        return 0

def create(workspace):
    return _ImageDS(workspace)
        
cdef class _ImageDSArray:
    cdef ImageDSArray* _array

    def __cinit__(self):
        self._array = NULL

    def __init__(self, path):
        self._array = new ImageDSArray(as_string(path))

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

def cell_attribute(name, attr_type_t attr_type, compression_t compression=NONE, compression_level=0):
    return Py_ImageDSAttribute(name, attr_type, compression, compression_level)

def define_array(name, dimensions, attributes):
    cdef imageds_array = _ImageDSArray(as_string(name))
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
 
