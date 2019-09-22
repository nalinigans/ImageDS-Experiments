# ImageDS-Experiments

## Using [Dicom Toolkit - DCMTK](https://dicom.offis.de/dcmtk)
### [DCMTK - Github](https://github.com/DCMTK/dcmtk)

## Using [Insight TookKit - ITK](https://www.itk.org/)

### Docker Install of ITK
Example 
* docker build -t try_itk:centos7 . # For building
* docker run -it try_itk:centos7 # For a bash shell in the docker image

### [Documentation](https://itk.org/ITKSoftwareGuide/html/Book2/ITKSoftwareGuide-Book2ch1.html) of ITK 

### ITK Usage with ImageDS
* First experiment to read from a DICOM image and write to ImageDS and vice versa. Start with
  * Implement itk's readers and writers
    * [itk::ImageFileReader](https://www.itk.org/Doxygen/html/classitk_1_1ImageFileReader.html)
    * [itk::ImageFileWriter](https://www.itk.org/Doxygen/html/classitk_1_1ImageFileWriter.html)
  * Figure out ImageDS for the images
    * One image per tiledb/array versus all images in one array.
    * How do we use itk? Do we read in the DICOM image, filter, register and segment before storing in ImageDS?
    * Can all the images be registered/segmented/scaled in a similar fashion?
    * If using Hadoop FileSystem HDFS, filesize is not a problem and we can store all the images together. Also, using tiledb we can write out one image/sets of images at a time, `concurrently` as each will write to a different fragment and consolidate if necessary later.
    
## To read...
### [Comparison](https://realpython.com/storing-images-in-python/) of using png/LMDB/HDF5 for storing images using python
### [Paper](https://etd.ohiolink.edu/!etd.send_file?accession=osu1524063297966335&disposition=inline) - Implementation of an image storage system using HDF5 as front end and tiledb as storage

