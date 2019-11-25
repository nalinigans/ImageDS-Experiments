import imageds
import numpy as np
import os
import shutil

ws = "workspace"
if os.path.exists(ws):
    shutil.rmtree(ws)
imageds.setup(ws)

x_dim = imageds.array_dimension("X", 0, 7, 2)
red = imageds.cell_attribute("Red", np.int16, imageds.compression_type.NONE, 0)
arr = imageds.define_array("PET_INT16", [x_dim], [red])

# write 1D array with 1 attribute
data = np.array([0, 1, 2, 3, 4, 5, 6, 7],  dtype=np.dtype(np.int16))
arr[:] = data

data = arr[:]
print(data)

x_dim = imageds.array_dimension("X", 0, 7, 2)
red = imageds.cell_attribute("Red", np.uint16, imageds.compression_type.NONE, 0)
arr = imageds.define_array("PET_UINT16", [x_dim], [red])

# write 1D array with 1 attribute
data = np.array([0, 1, 2, 3, 4, 5, 6, 7],  dtype=np.dtype(np.uint16))
arr[:] = data

data = arr[:]
print(data)
