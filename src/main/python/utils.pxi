# distutils: language = c++
# cython: language_level=3

cimport cython

from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdint cimport (int64_t, uint64_t, uintptr_t)

from imageds cimport ImageDSDimension

from cpython.version cimport PY_MAJOR_VERSION

from cpython.bytes cimport (PyBytes_GET_SIZE,
                            PyBytes_AS_STRING,
                            PyBytes_Size,
                            PyBytes_FromString,
                            PyBytes_FromStringAndSize)

import numpy as np
cimport numpy as np

# https://docs.scipy.org/doc/numpy/reference/c-api.array.html#c.import_array
np.import_array()

cdef unicode to_unicode(s):
    if type(s) is unicode:
        # Fast path for most common case(s).
        return <unicode>s
    elif isinstance(s, bytes):
        return s.decode('ascii')
    elif isinstance(s, unicode):
        # We know from the fast path above that 's' can only be a subtype here.
        # An evil cast to <unicode> might still work in some(!) cases,
        # depending on what the further processing does.  To be safe,
        # we can always create a copy instead.
        return unicode(s)
    else:
        raise TypeError("Could not convert to unicode.")

cdef string as_string(s):
    return PyBytes_AS_STRING(to_unicode(s).encode('UTF-8'))
