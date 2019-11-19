import array
import os
import shutil
import sys
import tempfile
import numpy as np

import imageds
from array import array

def run_test():
    print("Running tests...")
    print(imageds.version())
    
    imageds.setup("workspace")
    if not os.path.exists(tmp_dir + "/workspace"):
        print("Could not create workspace")
        print("Running tests DONE")

    # 1D array
    x_dim = imageds.array_dimension("XXX", 0, 7, 2)
    red = imageds.cell_attribute("Red", imageds.attr_type.UCHAR, imageds.compression_type.NONE, 0)
    arr = imageds.define_array("PET", [x_dim], [red])

    print(type(arr))

    # write 1D array with 1 attribute
    data = np.array([0, 1, 2, 3, 4, 5, 6, 7])
    arr[:] = data

    # 2D array with 1 attribute
    x_dim = imageds.array_dimension("XXX", 0, 4, 2)
    y_dim = imageds.array_dimension("YYY", 0, 4, 2)
    red = imageds.cell_attribute("Red", imageds.attr_type.UCHAR, imageds.compression_type.NONE, 0)
    arr = imageds.define_array("PET_2D", [x_dim, y_dim], [red])

    print(type(arr))

    # write 2D array with 1 attribute
    data = np.array(([1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12], [13, 14, 15, 16]))
    arr[:] = data

    print("we are here")
    data = arr[:]

    print("we are here 1")
    data = arr[1:3]

    print("we are here 2")
    try:
        data = arr[...]
    except Exception as e:
        print(e)

    
tmp_dir = tempfile.TemporaryDirectory().name

if not os.path.exists(tmp_dir):
    os.makedirs(tmp_dir)
else:
    sys.exit("Aborting as temporary directory seems to exist!")

os.chdir(tmp_dir)

run_test()

#print(tmp_dir)
shutil.rmtree(tmp_dir)
