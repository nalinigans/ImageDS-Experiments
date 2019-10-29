# distutils: language = c++
# cython: language_level=3

cimport cython

from libcpp.utility cimport pair
from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdint cimport (int64_t, uint64_t, uintptr_t)


from cpython.version cimport PY_MAJOR_VERSION

from cpython.bytes cimport (PyBytes_GET_SIZE,
														PyBytes_AS_STRING,
														PyBytes_Size,
														PyBytes_FromString,
														PyBytes_FromStringAndSize)

cdef unicode to_unicode(s):
	if type(s) is unicode:
		# Fast path for most common case(s).
		return <unicode>s
	elif PY_MAJOR_VERSION < 3 and isinstance(s, bytes):
		# Only accept byte strings as text input in Python 2.x, not in Py3.
		return (<bytes>s).decode('ascii')
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

cdef vector[string] as_vector(l):
	cdef vector[string] v
	for s in l:
		v.push_back(as_string(s))
	return v

