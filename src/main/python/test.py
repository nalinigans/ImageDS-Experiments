import array
import os
import shutil
import sys
import tempfile

import imageds
from array import array

def run_test():
    print("Running tests...")
    print(imageds.version())
    
    imageds_instance = imageds.create("workspace")
    if not os.path.exists(tmp_dir + "/workspace"):
        print("Could not create workspace")
        print("Running tests DONE")

    x_dim = imageds.array_dimension("XXX", 0, 7, 2)

    red = imageds.cell_attribute("Red", imageds.attr_type.UCHAR, imageds.compression_type.NONE, 0)

    arr = imageds.define_array("PET", [x_dim], [red])

    print(type(arr))
    
tmp_dir = tempfile.TemporaryDirectory().name

if not os.path.exists(tmp_dir):
    os.makedirs(tmp_dir)
else:
    sys.exit("Aborting as temporary directory seems to exist!")

os.chdir(tmp_dir)

run_test()

shutil.rmtree(tmp_dir)
