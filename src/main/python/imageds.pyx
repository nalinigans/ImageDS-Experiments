#distutils: language = c++
#cython: language_level=3

from __future__ import absolute_import, print_function

include "utils.pxi"

def version():
    version_string = imageds_version()
    return version_string


