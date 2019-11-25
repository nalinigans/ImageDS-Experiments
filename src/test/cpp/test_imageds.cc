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

  void make_temp_cwd() {
    chdir(get_temp_dir().c_str());
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
  // Check with absolute paths
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

  // Check relative paths
  make_temp_cwd();
  ImageDS imageds3(WORKSPACE+".another");
  CHECK(workspace_exists(WORKSPACE+".another"));

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

  ImageDSAttribute intensity_attr2("Intensity2", INT32, BLOSC_RLE, 3);
  CHECK(!intensity_attr2.name().compare("Intensity2"));
  CHECK(intensity_attr2.type() == INT32);
  CHECK(intensity_attr2.compression() == BLOSC_RLE);
  CHECK(intensity_attr2.compression_level() == 3);

  try {
    ImageDSAttribute("", INT8);
  } catch (const ImageDSException& e) {
    // Expected exception
  }
}

TEST_CASE("Test ImageDSArray", "[ImageDSArray]") {
  ImageDSArray array(ARRAY);
  array.add_dimension("X", 0, INT_MAX, 256);
  array.add_attribute("Intensity", UCHAR);
                      
  CHECK(!array.name().compare(ARRAY));
  CHECK(!array.path().compare(ARRAY));
  CHECK(array.dimensions().size() > 0);
  CHECK(array.attributes().size() > 0);

  ImageDSArray array1(ARRAY_IN_SUBDIR);
  array1.add_dimension("X", 0, INT_MAX, 256);
  array1.add_attribute("Intensity", UCHAR);
  CHECK(!array1.name().compare(ARRAY));
  CHECK(!array1.path().compare(ARRAY_IN_SUBDIR));
  CHECK(array1.dimensions().size() > 0);
  CHECK(array1.attributes().size() > 0);

  try {
    ImageDSArray err("");
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }

  std::vector<std::unique_ptr<ImageDSDimension>> dimensions;
  std::vector<std::unique_ptr<ImageDSAttribute>> attributes;

  try {
    ImageDSArray err("err", dimensions, attributes);
    FAIL();
  } catch (const ImageDSException& e) {
    // Expected exception
  }

  dimensions.push_back(std::unique_ptr<ImageDSDimension>(new ImageDSDimension("X", 0, INT_MAX, 256)));
  try {
    ImageDSArray err("err", dimensions, attributes);
    FAIL();
   } catch (const ImageDSException& e) {
    // Expected exception
   }

  attributes.push_back(std::unique_ptr<ImageDSAttribute>(new ImageDSAttribute("Intensity", UCHAR)));
  ImageDSArray new_array("new", dimensions, attributes);
  CHECK(new_array.name() == "new");
  CHECK(new_array.dimensions().size() == 1);
  CHECK(new_array.attributes().size() == 1);
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

  std::vector<std::unique_ptr<ImageDSDimension>> dimensions;
  std::vector<std::unique_ptr<ImageDSAttribute>> attributes;
  ImageDSDimension *dim = new ImageDSDimension("X", 0, 7, 2);
  dimensions.push_back(std::unique_ptr<ImageDSDimension>(dim));
  ImageDSAttribute *attr = new ImageDSAttribute("Intensity", UCHAR);
  attributes.push_back(std::unique_ptr<ImageDSAttribute>(attr));
  ImageDSArray array(ARRAY, dimensions, attributes);

  std::vector<char> buffer = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
  //std::vector<std::vector<char>> buffers;
  //buffers.push_back(buffer);

  //  CHECK(!imageds.to_array(array, buffers));
  std::vector<void *>buf;
  buf.push_back(buffer.data());
  std::vector<size_t>buf_size;
  buf_size.push_back(8);
  CHECK(!imageds.to_array(array, buf, buf_size));

  ImageDSArray array_info;
  CHECK(!imageds.array_info(ARRAY, array_info));
  //TODO: check array_info

  std::vector<char> from_array_buffer = {'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z'};
  buf.clear();
  buf.push_back(from_array_buffer.data());
  CHECK(!imageds.from_array(array, buf, buf_size));
  CHECK(buf_size[0] == 8);
  for (auto i=0ul; i<from_array_buffer.size(); i++) {
    CHECK(from_array_buffer[i] == 'A' + i);
  }

  ImageDSBuffers read_buffers = imageds.create_read_buffers(array);
  CHECK(read_buffers.get().size() == 1);
  CHECK(read_buffers.get_sizes().size() == 1);

  CHECK(!imageds.from_array(array, read_buffers.get(), read_buffers.get_sizes()));
  char *ptr = (char *)read_buffers.get()[0];
  for (auto i=0ul; i<read_buffers.get_sizes()[0]; i++) {
    CHECK(ptr[i] == 'A' + i);
  }
}

TEST_CASE_METHOD(TempDir, "Test to and from array 2D", "[to_from_array_2D]") {
  std::string workspace = append_paths(get_temp_dir(), WORKSPACE);
  
  ImageDS imageds(workspace);

  std::vector<std::unique_ptr<ImageDSDimension>> dimensions;
  std::vector<std::unique_ptr<ImageDSAttribute>> attributes;
  ImageDSDimension *dim = new ImageDSDimension("X", 0, 3, 2);
  dimensions.push_back(std::unique_ptr<ImageDSDimension>(dim));
  dim = new ImageDSDimension("Y", 0, 3, 2);
  dimensions.push_back(std::unique_ptr<ImageDSDimension>(dim));
  ImageDSAttribute *attr = new ImageDSAttribute("Intensity", CHAR);
  attributes.push_back(std::unique_ptr<ImageDSAttribute>(attr));
  ImageDSArray array(ARRAY, dimensions, attributes);

  std::vector<char> buffer = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q'};
  
  std::vector<void *>buf;
  buf.push_back(buffer.data());
  std::vector<size_t>buf_size;
  buf_size.push_back(16);
  CHECK(!imageds.to_array(array, buf, buf_size));

  ImageDSArray array_info;
  CHECK(!imageds.array_info(ARRAY, array_info));
  //TODO: check array_info

  std::vector<char> from_array_buffer = {'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z', 'Z'};
  buf.clear();
  buf.push_back(from_array_buffer.data());
  CHECK(!imageds.from_array(array, buf, buf_size));
  CHECK(buf_size[0] == 16);
  for (auto i=0ul; i<from_array_buffer.size(); i++) {
    CHECK(from_array_buffer[i] == 'A' + i);
  }

  ImageDSBuffers read_buffers = imageds.create_read_buffers(array);
  CHECK(read_buffers.get().size() == 1);
  CHECK(read_buffers.get_sizes().size() == 1);
  CHECK(read_buffers.get_sizes()[0] == 16);

  CHECK(!imageds.from_array(array, read_buffers.get(), read_buffers.get_sizes()));
  char *ptr = (char *)read_buffers.get()[0];
  for (auto i=0ul; i<read_buffers.get_sizes()[0]; i++) {
    CHECK(ptr[i] == 'A' + i);
  }

  // TODO: Support subarray's better. This code is just for now
  attributes.clear();
  dimensions.clear();
  dim = new ImageDSDimension("X", 0, 2, 1);
  dimensions.push_back(std::unique_ptr<ImageDSDimension>(dim));
  dim = new ImageDSDimension("Y", 0, 2, 1);
  dimensions.push_back(std::unique_ptr<ImageDSDimension>(dim));
  attr = new ImageDSAttribute("Intensity", CHAR);
  attributes.push_back(std::unique_ptr<ImageDSAttribute>(attr));
  ImageDSArray subarray(ARRAY, dimensions, attributes);

  char *bytes = new char[10];
  bytes[9]=0;
  buf.clear();
  buf_size.clear();
  buf.push_back(bytes);
  buf_size.push_back(9);

  CHECK(!imageds.from_array(subarray, buf, buf_size));

  std::string expected("ABCEFGIJK");
  CHECK(expected.compare(bytes) == 0);
}


