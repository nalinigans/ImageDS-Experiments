/**
 * @file test_imageds.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2019 Omics Data Automation, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION Tests for imageds.cc
 */

#include "catch.h"
#include "error.h"
#include "imageds.h"
#include "tiledb_utils.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

const std::string WORKSPACE = "imageds_test_ws";
const std::string ARRAY = "imageds_test_array";
const std::string ARRAY_IN_SUBDIR = "path1/path2/imageds_test_array";
const std::string ABSOLUTE_ARRAY_PATH = "/path1/path2/imageds_test_array";

using namespace TileDBUtils;

class TempDir {
 public:
  TempDir() {
    create_temp_directory();
  }

  ~TempDir() {
    delete_dir(get_temp_dir());
  }

  const std::string& get_temp_dir() {
    return tmp_dirname_;
  }
  
 private:
  std::string tmp_dirname_;

  void create_temp_directory() {
    std::string dirname_pattern("ImageDSXXXXXX");
    const char *tmp_dir = getenv("TMPDIR");
    if (tmp_dir == NULL) {
      tmp_dir = P_tmpdir; // defined in stdio
    }
    assert(tmp_dir != NULL);
    tmp_dirname_ = mkdtemp(const_cast<char *>((append_paths(tmp_dir, dirname_pattern)).c_str()));
  }
};

TEST_CASE_METHOD(TempDir, "Test constructor", "[test_constr]") {
  std::string workspace = append_paths(get_temp_dir(), WORKSPACE);
  
  // with default overwrite=false
  CHECK(!workspace_exists(workspace));
  
  ImageDS imageds(workspace);
  CHECK(workspace_exists(workspace));

  // Create a random file in the workspace dir to check workspace overwrite
  std::string write_buffer("TEST");
  CHECK(!write_file(workspace+"/test_file", write_buffer.data(), write_buffer.size()));
  CHECK(is_file(workspace+"/test_file"));

  try {
    ImageDS imageds1(workspace); // should throw exception as workspace already exists
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }

  ImageDS imageds2(workspace, true);
  // re-created workspace should not contain random file
  CHECK(!is_file(workspace+"/test_file"));

  delete_dir(workspace);
}

TEST_CASE("Test ImageDSDimension", "[ImageDSDimension]") {
  ImageDSDimension dimX("X", 0, INT_MAX, 256);
  CHECK(!dimX.name().compare("X"));
  CHECK(dimX.start() == 0);
  CHECK(dimX.end() == INT_MAX);
  CHECK(dimX.tile_extent() == 256);
  
  ImageDSDimension dimY("Y", 0, 2047, 256);
  CHECK(!dimY.name().compare("Y"));
  CHECK(dimY.start() == 0);
  CHECK(dimY.end() == 2047);
  CHECK(dimY.tile_extent() == 256);

  try {
    ImageDSDimension err("", 0, 0, 0);
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }

  try {
    ImageDSDimension err("err", 0, 0, 0);
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }

  try {
    ImageDSDimension err("err", 0, 1, 2);
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }
}

TEST_CASE("Test ImageDSAttribute", "[ImageDSAttribute]") {
  ImageDSAttribute intensity_attr("Intensity", UCHAR);
  CHECK(!intensity_attr.name().compare("Intensity"));
  CHECK(intensity_attr.type() == UCHAR);
  CHECK(intensity_attr.compression() == NONE);
  CHECK(intensity_attr.compression_level() == 0);

  ImageDSAttribute intensity_attr1("Intensity1", UCHAR, GZIP);
  CHECK(!intensity_attr1.name().compare("Intensity1"));
  CHECK(intensity_attr1.type() == UCHAR);
  CHECK(intensity_attr1.compression() == GZIP);
  CHECK(intensity_attr1.compression_level() == 0);

  ImageDSAttribute intensity_attr2("Intensity2", INT_32, BLOSC_RLE, 3);
  CHECK(!intensity_attr2.name().compare("Intensity2"));
  CHECK(intensity_attr2.type() == INT_32);
  CHECK(intensity_attr2.compression() == BLOSC_RLE);
  CHECK(intensity_attr2.compression_level() == 3);

  try {
    ImageDSAttribute("", INT_8);
  } catch (const ImageDSException& e) {
    // Expected exception
  }
}

TEST_CASE("Test ImageDSArray", "[ImageDSArray]") {
  std::vector<ImageDSDimension> dimensions;
  std::vector<ImageDSAttribute> attributes;
  ImageDSDimension dim("X", 0, INT_MAX, 256);
  dimensions.push_back(dim);
  ImageDSAttribute attr("Intensity", UCHAR);
  attributes.push_back(attr);

  ImageDSArray array(ARRAY, dimensions, attributes);
  CHECK(!array.name().compare(ARRAY));
  CHECK(!array.path().compare(ARRAY));
  CHECK(array.dimensions().size() > 0);
  CHECK(array.attributes().size() > 0);

  ImageDSArray array1(ARRAY_IN_SUBDIR, dimensions, attributes);
  CHECK(!array1.name().compare(ARRAY));
  CHECK(!array1.path().compare(ARRAY_IN_SUBDIR));
  CHECK(array1.dimensions().size() > 0);
  CHECK(array1.attributes().size() > 0);

  try {
    ImageDSArray err("", {}, {});
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }

   try {
    ImageDSArray err("err", {}, {});
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }

   try {
    ImageDSArray err("err", dimensions, {});
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
   }
}

TEST_CASE_METHOD(TempDir, "Test array_info", "[non_existent_array_info]") {
  std::string workspace = append_paths(get_temp_dir(), WORKSPACE);
  
  ImageDS imageds(workspace);

  ImageDSArray array;
  int rc = imageds.array_info(ARRAY, array);
  CHECK(rc != IMAGEDS_OK);
  bool check = (errno == EIO) || (errno == ENOENT);
  CHECK(check);
}

TEST_CASE_METHOD(TempDir, "Test to and from array", "[to_from_array]") {
  std::string workspace = append_paths(get_temp_dir(), WORKSPACE);
  
  ImageDS imageds(workspace);

  std::vector<ImageDSDimension> dimensions;
  std::vector<ImageDSAttribute> attributes;
  ImageDSDimension dim("X", 0, 8, 2);
  dimensions.push_back(dim);
  ImageDSAttribute attr("Intensity", UCHAR);
  attributes.push_back(attr);
  ImageDSArray array(ARRAY, dimensions, attributes);

  std::vector<char> buffer = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
  std::vector<std::vector<char>> buffers;
  buffers.push_back(buffer);

  CHECK(!imageds.to_array(array, buffers));

  ImageDSArray array_info;
  CHECK(!imageds.array_info(ARRAY, array_info));
  //TODO: check array_info

  std::vector<char> from_array_buffer = {'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z'};
  from_array_buffer.resize(8);
  buffers.clear();
  buffers.push_back(from_array_buffer);

  CHECK(!imageds.from_array(array, buffers));
  CHECK(buffers.size() == 1);
  CHECK(buffers[0].size() == 8);
  CHECK(buffers[0][0] == 'A');
  CHECK(buffers[0][7] == 'H');
}

/*class ImageDSTestFixture {
 public:
  void *handle;
  compression_t compression = NONE;

  bool is_dir(const std::string& dir) {
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    return !stat(dir.c_str(), &st) && S_ISDIR(st.st_mode);
  }
  
  bool is_file(const std::string& file) {
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    return !stat(file.c_str(), &st) && S_ISREG(st.st_mode);
  }

  ImageDSTestFixture() {
    CHECK_RC(imageds_connect(WORKSPACE, &handle), IMAGEDS_OK);
    CHECK(is_dir(WORKSPACE));
    REQUIRE(handle != NULL);
  }
  
  ~ImageDSTestFixture() {
    CHECK_RC(imageds_disconnect(handle), IMAGEDS_OK);
    // Remove the temporary workspace
    std::string command = "rm -rf ";
    command.append(WORKSPACE);
    int rc = system(command.c_str());
    CHECK_RC(rc, 0);
  }

  void check_array_info(const char *full_array_path, imageds_region_t *expected_region, imageds_array_t *expected_array) {
    imageds_region_t region;
    imageds_array_t array;
    CHECK_RC(imageds_array_info(full_array_path, &region, &array), IMAGEDS_OK);

    // Region check
    CHECK(region.num_dimensions == expected_region->num_dimensions);
    for (auto i=0; i<region.num_dimensions; i++) {
      CHECK(strcmp(region.dimension_name[i], expected_region->dimension_name[i]) == 0);
      CHECK(region.start[i] == expected_region->start[i]);
      CHECK(region.end[i] == expected_region->end[i]);
    }

    // Array check
    CHECK(array.num_attributes == expected_array->num_attributes);
    for (auto i=0; i<array.num_attributes; i++) {
      CHECK(strcmp(array.attribute_names[i], expected_array->attribute_names[i]) == 0);
      CHECK(array.attribute_types[i] == expected_array->attribute_types[i]);
      CHECK(array.compression[i] == expected_array->compression[i]);
    }
  }

  void test_load_read(const char *array_path) {
    imageds_array_t array;
    array.name = array_path;
    array.num_attributes = 1;
    const char *attr_names[] = {"attr1"};
    array.attribute_names = attr_names;
    attr_types_t attr_types[] = {UCHAR};
    array.attribute_types = attr_types;
    compression_t compress[] = {NONE};
    array.compression = compress;
    array.compression_level = NULL;
    array.tile_extents = 100;

    imageds_region_t region;
    region.num_dimensions = 1;
    const char *dim_names[] = {"dim1"};
    region.dimension_name = dim_names;
    uint64_t starts[] = {0};
    uint64_t ends[] = {7};
    region.start = starts;
    region.end = ends;

    char buf_values[] = {'0', '1', '2', '3', '4', '5', '6', '7'};
    imageds_buffer_t buffer;
    buffer.buffer = reinterpret_cast<void *>(buf_values);
    buffer.size = 8;
  
    CHECK_RC(imageds_load_array(ImageDSTestFixture::handle, &region, &array, buffer), IMAGEDS_OK);
    std::string path(WORKSPACE);
    
    std::string full_array_path = path.append("/").append(array_path);
    REQUIRE(is_dir(full_array_path));

    check_array_info(full_array_path.c_str(), &region, &array);;

    void **ret_buffer;
    size_t ret_region_size;
    uint64_t ret_region_size_x;
    CHECK_RC(imageds_read_array(ImageDSTestFixture::handle, &region, &array, &ret_buffer, &ret_region_size, &ret_region_size_x), IMAGEDS_OK);

    CHECK(ret_region_size == 8);
    REQUIRE(ret_buffer != NULL);
    char **ret_buf = reinterpret_cast<char **>(ret_buffer);
    for (auto i=0; i<8; i++) {
      CHECK((ret_buf[0][i]-'0')==i); 
    }

    CHECK_RC(imageds_free_array_buffer(ret_buffer), IMAGEDS_OK);
  }
};

void test_connect_with_null() {
  CHECK(imageds_connect(NULL, NULL));
  CHECK(imageds_connect(NULL,  NULL));
  CHECK(imageds_connect("x", NULL)); 
  CHECK(imageds_disconnect(NULL));
}

TEST_CASE_METHOD(ImageDSTestFixture, "Test constructor/destructor", "[test_constructor_destructor]") {
  REQUIRE(ImageDSTestFixture::handle != NULL);
}

TEST_CASE_METHOD(ImageDSTestFixture, "Test load/read", "[test_load_read]") {
  REQUIRE(ImageDSTestFixture::handle != NULL);
  ImageDSTestFixture::test_load_read(ARRAY);
}

TEST_CASE_METHOD(ImageDSTestFixture, "Test load/read with array in subdir", "[test_load_read_array_in_subdir]") {
  REQUIRE(ImageDSTestFixture::handle != NULL);
  ImageDSTestFixture::test_load_read(ARRAY_IN_SUBDIR);
}

TEST_CASE_METHOD(ImageDSTestFixture, "Test load with absolute array path", "[test_load_with_abspath_array]") {
  REQUIRE(ImageDSTestFixture::handle != NULL);
  imageds_array_t array;
  array.name = ABSOLUTE_ARRAY_PATH;
  CHECK_RC(imageds_load_array(ImageDSTestFixture::handle, NULL, &array), IMAGEDS_ERR);
}

TEST_CASE_METHOD(ImageDSTestFixture, "Test load/read with gzip", "[test_load_read_gzip]") {
  Require(ImageDSTestFixture::handle != NULL);
  ImageDSTestFixture::compression = GZIP;
  ImageDSTestFixture::test_load_read(ARRAY);
}
*/
