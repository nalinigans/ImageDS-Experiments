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
    print("Test 1D array")
    x_dim = imageds.array_dimension("X", 0, 7, 2)
    red = imageds.cell_attribute("Red", np.ubyte, imageds.compression_type.NONE, 0)
    arr = imageds.define_array("PET", [x_dim], [red])
    # write 1D array with 1 attribute
    print("\tWrite 1D array")
    data = np.array([0, 1, 2, 3, 4, 5, 6, 7],  dtype=np.dtype(np.ubyte))
    arr[:] = data
    # read 1D array with 1 attribute
    print("\tRead 1D array")
    data = arr[:]
    print(data)

    # 2D array with 1 attribute
    print("Test 2D array")
    x_dim = imageds.array_dimension("X", 0, 3, 2)
    y_dim = imageds.array_dimension("Y", 0, 3, 2)
    red = imageds.cell_attribute("Red", np.uint32, imageds.compression_type.NONE, 0)
    arr = imageds.define_array("PET_2D", [x_dim, y_dim], [red])  
    # write 2D array with 1 attribute
    print("\tWrite 2D array")
    data = np.array(([1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12], [13, 14, 15, 16]), dtype=np.dtype(np.uint32))
    arr[:] = data
    # read 2D array with 1 attribute
    print("\tRead 2D array")
    data = arr[:]
    print(data)

    try:
        data = arr[1:3]
        print(data)
    except Exception as e:
        print("Expected exception: " + str(e))

    try:
        data = arr[...]
    except Exception as e:
        print("Expected exception: " + str(e))

    
tmp_dir = tempfile.TemporaryDirectory().name

if not os.path.exists(tmp_dir):
    os.makedirs(tmp_dir)
else:
    sys.exit("Aborting as temporary directory seems to exist!")

os.chdir(tmp_dir)

run_test()

#print(tmp_dir)
shutil.rmtree(tmp_dir)
