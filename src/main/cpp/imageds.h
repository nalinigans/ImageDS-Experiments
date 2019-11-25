/*
 * @ imageds.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2019 Nalini Ganapati
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
 * @section DESCRIPTION ImageDS data structures
 *
 */

#ifndef __IMAGEDS_H__
#define __IMAGEDS_H__

#include "error.h"

#include <stdarg.h>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <vector>

#if (defined __GNUC__ && __GNUC__ >= 4) || defined __INTEL_COMPILER
#  define IMAGEDS_PUBLIC __attribute__((visibility("default")))
#else
#  define IMAGEDS_PUBLIC
#endif

IMAGEDS_PUBLIC std::string imageds_version();

typedef enum imageds_attr_type_t {
  CHAR=4,         // TILEDB_CHAR
  UCHAR=5,        // TILEDB_INT8
  INT8=5,         // TILEDB_INT8
  INT16=7,        // TILDB_INT16
  INT32=0,        // TILEDB_INT32
  INT64=1,        // TILEDB_INT64
  UINT8=6,        // TILEDB_UINT8
  UINT16=8,       // TILDB_UINT16
  UINT32=9,       // TILEDB_UINT32
  UINT64=10,      // TILEDB_UINT64
  FLOAT32=2,      // TILEDB_FLOAT32
  FLOAT64=3       // TILEDB_FLOAT64
} attr_type_t;

typedef enum imageds_compression_t {
  NONE=0,         // TILEDB_NO_COMPRESSION
  GZIP=1,         // TILEDB_GZIP
  ZSTD=2,         // TILEDB_ZSTD
  LZ4=3,          // TILEDB_LZ4
  BLOSC=4,        // TILEDB_BLOSC
  BLOSC_LZ4=5,    // TILEDB_BLOSC_LZ4
  BLOSC_LZ4HC=6,  // TILEDB_BLOSC_LZ4HC
  BLOSC_SNAPPY=7, // TILEDB_BLOSC_SNAPPY
  BLOSC_ZLIB=8,   // TILEDB_BLOSC_ZLIB
  BLOSC_ZSTD=9,   // TILEDB_BLOSC_ZSTD
  BLOSC_RLE=10,   // TILEDB_RLE
} compression_t;

static std::string remove_trailing_slash(const std::string& path) {
  if (path[path.size()-1] == '/') {
    return path.substr(0, path.size()-1);
  } else {
    return path;
  }
}

static std::string pathname(const std::string& path) {
  std::string no_trailing_slash_path = remove_trailing_slash(path);
  const size_t last_slash_idx = no_trailing_slash_path.rfind('/');
  if (last_slash_idx != std::string::npos) {
    return no_trailing_slash_path.substr(last_slash_idx+1);
  } else {
    return no_trailing_slash_path;
  }
}

static std::string append_paths(const std::string& path1, const std::string& path2) {
  VERIFY(path2[0] != '/' && "Second path argument to append_paths cannot be absolute");
  return remove_trailing_slash(path1) + "/" + path2;
}

static bool is_absolute_path(const std::string& path) {
  return path[0] == '/';
}

class IMAGEDS_PUBLIC ImageDSDimension {
 public:
  const std::string m_name;
  uint64_t m_start;
  uint64_t m_end;
  uint64_t m_tile_extent;

  ImageDSDimension(const std::string& name, uint64_t start, uint64_t end, uint64_t tile_extent)
      : m_name(name), m_start(start), m_end(end), m_tile_extent(tile_extent) {
    VERIFY(!name.empty() && "Dimension name specified cannot be empty");
    VERIFY((start != end) && "Invalid specified start/end for dimension");
    VERIFY((tile_extent != 0) && "Invalid specified tile extent for dimension, cannot be zero");
    VERIFY((tile_extent < (end-start)) && "Invalid specified tile extent for dimension, cannot exceed dimension length");
  }

  // Delete copy constructor
  ImageDSDimension(const ImageDSDimension& other) = delete;  
  ImageDSDimension(ImageDSDimension& other) = delete;  

  const std::string name() {
    return m_name;
  }

  uint64_t start() {
    return m_start;
  }

  uint64_t end() {
    return m_end;
  }

  uint64_t tile_extent() {
    return m_tile_extent;
  }
};

class IMAGEDS_PUBLIC ImageDSAttribute {
 public:
  const std::string m_name;
  attr_type_t m_type;
  compression_t m_compression;
  int m_compression_level;

  ImageDSAttribute(const std::string& name, attr_type_t type, compression_t compression=NONE, int compression_level=0)
      : m_name(name), m_type(type), m_compression(compression), m_compression_level(compression_level) {
    VERIFY(!name.empty() && "Attribute name specified cannot be empty");
  }

  // Delete copy constructor
  ImageDSAttribute(const ImageDSAttribute& other) = delete;
  ImageDSAttribute(ImageDSAttribute& other) = delete;

  const std::string name() {
    return m_name;
  }

  attr_type_t type() {
    return m_type;
  }

  compression_t compression() {
    return m_compression;
  }

  int compression_level() {
    return m_compression_level;
  }
};

class IMAGEDS_PUBLIC ImageDSArray {
 public:
  std::string m_path;
  std::string m_name;
  std::vector<std::unique_ptr<ImageDSDimension>> m_dimensions;
  std::vector<std::unique_ptr<ImageDSAttribute>> m_attributes;

  ImageDSArray() {}

  ImageDSArray(const std::string& path) : m_path(path) {
    VERIFY(!path.empty() && "Array Path specified cannot be empty");
    m_name = pathname(path);
  };

  ImageDSArray(const std::string& path, std::vector<std::unique_ptr<ImageDSDimension>>& dimensions, std::vector<std::unique_ptr<ImageDSAttribute>>& attributes)
      : m_path(path) {
    VERIFY(!path.empty() && "Array Path specified cannot be empty");
    VERIFY(dimensions.size()>0 && "Array Dimensions required to be specified");
    VERIFY(attributes.size()>0 && "Array Attributes required to be specified");
    m_name = pathname(path);
    m_dimensions = std::move(dimensions);
    m_attributes = std::move(attributes);
  }

  const std::string name() {
    return m_name;
  }

  const std::string path() {
    return m_path;
  }

  const std::vector<std::unique_ptr<ImageDSDimension>>& dimensions() {
    return m_dimensions;
  }

  const std::vector<std::unique_ptr<ImageDSAttribute>>& attributes() {
    return m_attributes;
  }

  void add_dimension(const std::string& name, uint64_t start, uint64_t end, uint64_t tile_extent) {
    m_dimensions.push_back(std::unique_ptr<ImageDSDimension>(new ImageDSDimension(name, start, end, tile_extent)));
  }

  void add_attribute(const std::string& name, attr_type_t type, compression_t compression=NONE, int compression_level=0) {
    m_attributes.push_back(std::unique_ptr<ImageDSAttribute>(new ImageDSAttribute(name, type, compression, compression_level)));
  }
};

class IMAGEDS_PUBLIC ImageDSBuffers {
 public:
  std::vector<void *> m_buffers;
  std::vector<size_t> m_buffer_sizes;

  void add(void *buffer, size_t buffer_size) {
    m_buffers.push_back(buffer);
    m_buffer_sizes.push_back(buffer_size);
  }

  std::vector<void *> get() {
    return m_buffers;
  }

  std::vector<size_t> get_sizes() {
    return m_buffer_sizes;
  }
};

class IMAGEDS_PUBLIC ImageDS {
 public:
  ImageDS(const std::string& workspace, const bool overwrite=false, const bool disable_file_locking=false);

  ~ImageDS();

  int array_info(const std::string& array_path, ImageDSArray& array);

  int to_array(ImageDSArray& array, const std::vector<void *> buffers, const std::vector<size_t> buffer_sizes);

  ImageDSBuffers create_read_buffers(ImageDSArray& array);

  int from_array(ImageDSArray& array, std::vector<void *> buffers, std::vector<size_t> buffer_sizes);

 private:
  int create_tiledb_groups(const std::string& array_path);
  int setup_tiledb_schema(ImageDSArray& array);

  std::string m_workspace;
  std::string m_working_dir;
  void* m_tiledb_ctx;
};

#endif //__IMAGEDS_H__
