# distutils: language = c++
# cython: language_level=3

from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdint cimport (uint8_t, uint64_t)

cdef extern from "imageds.h":
  cdef string imageds_version()

  #constants

  ctypedef enum attr_type_t:
    UCHAR=4,
    INT_8=4,
    INT_32=0,
    INT_64=1

  ctypedef enum compression_t:
    NONE=0
    GZIP=1
    ZSTD=2
    LZ4=3
    BLOSC=4
    BLOSC_LZ4=5
    BLOSC_LZ4HC=6
    BLOSC_SNAPPY=7
    BLOSC_ZLIB=8
    BLOSC_ZSTD=9
    BLOSC_RLE=10

  cdef cppclass ImageDSDimension:
    ImageDSDimension(string, uint64_t, uint64_t, uint64_t) except +
    pass

  cdef cppclass ImageDSAttribute:
    ImageDSAttribute(string, attr_type_t, compression_t, int) except +
    ImageDSAttribute(string, attr_type_t, compression_t) except +
    ImageDSAttribute(string, attr_type_t) except +
    pass

  cdef cppclass ImageDSArray:
    ImageDSArray()
    ImageDSArray(string) except +
    ImageDSArray(string, vector[ImageDSDimension], vector[ImageDSAttribute]) except +
    void add_dimension(string, uint64_t, uint64_t, uint64_t)
    void add_attribute(string, attr_type_t, compression_t, int)
    pass

  cdef cppclass ImageDSBuffers:
    void add(void*, size_t)
    vector[void*] get()
    pass

  cdef cppclass ImageDS:
    ImageDS(string, bool, bool) except +
    ImageDS(string, bool) except +
    ImageDS(string) except +
    int array_info(string, ImageDSArray)
    int to_array(ImageDSArray, vector[void *], vector[size_t])
    ImageDSBuffers create_read_buffers(ImageDSArray)
    from_array(ImageDSArray, vector[void *], vector[size_t])
    pass
