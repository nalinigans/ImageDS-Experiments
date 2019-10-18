/**
 * @file imageds.cc
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
 * @section DESCRIPTION ImageDS C++ Implementation
 */

#include "error.h"
#include "imageds.h"
#include "tiledb.h"
#include "tiledb_constants.h"
#include "tiledb_storage.h"
#include "tiledb_utils.h"

#include <assert.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string.h>

#define TILEDB_CTX reinterpret_cast<TileDB_CTX*>(m_tiledb_ctx)

ImageDS::ImageDS(const std::string& workspace, const bool overwrite, const bool disable_file_locking)
    : m_workspace(workspace) {
  TileDB_CTX* tiledb_ctx;
  VERIFY(!TileDBUtils::initialize_workspace(&tiledb_ctx, workspace, overwrite, disable_file_locking) && "Could not create TileDB workspace");
  m_tiledb_ctx = reinterpret_cast<void*>(tiledb_ctx);
  m_working_dir = parent_dir(workspace);
  if (m_working_dir.empty()) {
    m_working_dir = current_working_dir(tiledb_ctx);
  }
}

ImageDS::~ImageDS() {
  if (tiledb_ctx_finalize(TILEDB_CTX)) {
    std::cerr << "Could not finalize TileDB:" << tiledb_errmsg << std::endl; 
  }
}

int ImageDS::array_info(const std::string& array_path, ImageDSArray& array) {
  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_workspace));
  RETURN_EIO_IF_ERROR(!is_array(TILEDB_CTX, array_path));

  TileDB_Array* tiledb_array;
  RETURN_EINVAL_IF_ERROR(
      tiledb_array_init(TILEDB_CTX,
                        &tiledb_array,
                        array_path.c_str(),
                        TILEDB_ARRAY_READ,
                        NULL, // Entire domain
                        NULL, // All attributes
                        0));

  TileDB_ArraySchema array_schema;
  RETURN_EINVAL_IF_ERROR(tiledb_array_get_schema(tiledb_array, &array_schema));
  if (!array_schema.dense_) {
    errno = ECANCELED;
    return IMAGEDS_ERR;
  }

  array.m_name = array_schema.array_name_;
  for (auto i=0; i<array_schema.attribute_num_; i++) {
    ImageDSAttribute attribute(array_schema.attributes_[i],
                               (attr_type_t)array_schema.types_[i],
                               (compression_t)array_schema.compression_[i],
                               array_schema.compression_level_[i]);
    array.m_attributes.push_back(attribute);
  }
  
  uint64_t *domain =  (uint64_t *)array_schema.domain_;
  int64_t *tile_extents = (int64_t *)array_schema.tile_extents_;
  for (auto i=0; i<array_schema.dim_num_; i++) {
    ImageDSDimension dimension(array_schema.dimensions_[i],
                               domain[i],
                               domain[i+1],
                               tile_extents[i]);
    array.m_dimensions.push_back(dimension);
  }

  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_working_dir));
  return IMAGEDS_OK;
}

int ImageDS::to_array(ImageDSArray& array, const std::vector<std::vector<char>>& buffers) {
  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_workspace));
  
  if (is_array(TILEDB_CTX, array.m_path)) {
    // TODO: Validate existing schema
  } else {
    RETURN_ECANCELED_IF_ERROR(setup_tiledb_schema(array));
  }

  TileDB_Array* tiledb_array;
  RETURN_EINVAL_IF_ERROR(tiledb_array_init(TILEDB_CTX,
                                           &tiledb_array,
                                           array.m_path.c_str(),
                                           TILEDB_ARRAY_WRITE_SORTED_ROW,
                                           NULL, // Entire domain
                                           NULL, // All attributes
                                           0));
  std::vector<const char *> buf;
  std::vector<size_t> buf_size;
  buf.resize(buffers.size());
  buf_size.resize(buffers.size());
  for (auto i=0u; i<buffers.size(); i++) {
    buf[i] = buffers[i].data();
    buf_size[i] = buffers[i].size()*sizeof(char);
  }

  RETURN_ECANCELED_IF_ERROR(tiledb_array_write(tiledb_array,
                                               reinterpret_cast<const void**>(buf.data()),
                                               buf_size.data()));

  //TODO: Check for overflow

  RETURN_ECANCELED_IF_ERROR(tiledb_array_finalize(tiledb_array));

  //TODO: Serialize TileDB_ArraySchema as JSON.
  //TileDB_ArraySchema schema;
  //tiledb_array_get_schema(tiledb_array, &schema);

  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_working_dir));
  return IMAGEDS_OK;
}

int ImageDS::from_array(ImageDSArray& array, std::vector<std::vector<char>>& buffers) {
  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_workspace));
  
  TileDB_Array* tiledb_array;
 
  size_t size = 0;
  size_t dimensions_length = array.m_dimensions.size();
  if (dimensions_length > 0) {
    size = 1;
    uint64_t subarray[dimensions_length*2];
    for (auto i=0ul; i<dimensions_length; i++) {
      subarray[i*2] = array.m_dimensions[i].m_start;
      subarray[i*2+1] = array.m_dimensions[i].m_end;
      size *= (array.m_dimensions[i].m_end - array.m_dimensions[i].m_start + 1);
    }
    RETURN_EINVAL_IF_ERROR(tiledb_array_init(TILEDB_CTX, &tiledb_array,
                                             array.m_path.c_str(),
                                             TILEDB_ARRAY_READ_SORTED_ROW,
                                             subarray, 
                                             NULL, // All attributes
                                             0));
  } else {
    RETURN_EINVAL_IF_ERROR(tiledb_array_init(TILEDB_CTX, &tiledb_array,
                                             array.m_path.c_str(),
                                             TILEDB_ARRAY_READ_SORTED_ROW,
                                             NULL, 
                                             NULL, // All attributes
                                             0));
  }
  
  TileDB_ArraySchema schema;
  tiledb_array_get_schema(tiledb_array, &schema);

  if (!size) {
    size = 1;
    uint64_t* domain = (uint64_t*) schema.domain_;
    for (int i=0; i<schema.dim_num_; i++) {
      uint64_t start = reinterpret_cast<uint64_t>(*domain);
      domain++;
      uint64_t end = reinterpret_cast<uint64_t>(*domain);
      domain++;
      size *= ((end-start)+1);
    }
  }

  std::vector<char*> buf;
  std::vector<size_t> buf_size;
  buf.resize(buffers.size());
  buf_size.resize(buffers.size());
  for (int i=0; i<schema.attribute_num_; i++) {
    buf[i] = buffers[i].data();
    buf_size[i] = buffers[i].size()*sizeof(char);
    /*    switch (array.m_attributes[i].m_type) {
      buf[i] = buffers[i].data();
      case UCHAR:
        buf_size[i].size()*sizeof(char)
	break;
      case INT_32:
        buf_size[i].size()*sizeof(char)
        break;
      default:
        throw std::runtime_error("Not yet implemented!");
        } */
  }

  RETURN_ECANCELED_IF_ERROR(tiledb_array_read(tiledb_array,
                                              reinterpret_cast<void**>(buf.data()),
                                              buf_size.data()));
  
  // RETURN_ECANCELED_IF_ERROR(tiledb_array_free_schema(&schema));

  for (int i=0; i<schema.attribute_num_; i++) {
    if (tiledb_array_overflow(tiledb_array, i) == 1) {
      throw std::runtime_error("Buffer overflow encountered");
    }
  }

  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_working_dir));
  return IMAGEDS_OK;
}


int ImageDS::create_tiledb_groups(const std::string& array_path) {
  if (array_path[0] == '/') {
    errno = EINVAL;
    return IMAGEDS_ERR;
  }
  if (array_path.find("/") != std::string::npos) {
    std::string path_segment;
    std::istringstream path(array_path.substr(0, array_path.rfind("/")));
    std::string group(m_workspace);
    while (std::getline(path, path_segment, '/')) {
      group.append("/").append(path_segment);
      RETURN_EINVAL_IF_ERROR(tiledb_group_create(TILEDB_CTX, group.c_str()));
    }
  }
  return IMAGEDS_OK;
}

int ImageDS::setup_tiledb_schema(ImageDSArray& array) {
  RETURN_EINVAL_IF_ERROR(create_tiledb_groups(array.m_path));

  std::string array_path = array.m_path;

  int length = array.m_dimensions.size();
  const char *dimensions[length];
  uint64_t domain[length*2];
  int64_t tile_extents[length];
  for (int i=0; i<length; i++) {
    dimensions[i] = array.m_dimensions[i].m_name.c_str();
    domain[i*2] = array.m_dimensions[i].m_start;
    domain[i*2+1] =  array.m_dimensions[i].m_end;
    tile_extents[i] = array.m_dimensions[i].m_tile_extent;
  }

  length = array.m_attributes.size();
  const char *attribute_names[length];
  int attribute_types[length+1]; // +1 for coordinates
  int attribute_compression[length+1]; // +1 for coordinates
  int attribute_compression_level[length+1]; // +1 for coordinates
  int num_cells_per_attr[length];
  for (int i=0; i<length; i++) {
    attribute_names[i] = array.m_attributes[i].m_name.c_str();
    attribute_types[i] =  array.m_attributes[i].m_type;
    num_cells_per_attr[i] = 1; // TODO change based on attribute type
    attribute_compression[i] = array.m_attributes[i].m_compression;
    attribute_compression_level[i] =  array.m_attributes[i].m_compression_level;
  }
  // For co-ordinates
  attribute_types[length] = TILEDB_INT64;
  attribute_compression[length] = TILEDB_NO_COMPRESSION;
  attribute_compression[length] = 1;


  TileDB_ArraySchema array_schema;
  RETURN_EINVAL_IF_ERROR(
      tiledb_array_set_schema(&array_schema,
                              array_path.c_str(),
                              attribute_names,
                              array.m_attributes.size(),
                              2, // Capacity for data tiles??
                              TILEDB_ROW_MAJOR, // Cell Order
                              num_cells_per_attr,
                              attribute_compression,
                              attribute_compression_level,
                              1, // Dense Array
                              dimensions,
                              array.m_dimensions.size(),
                              domain,
                              array.m_dimensions.size()*2*sizeof(uint64_t), //domain length
                              tile_extents,
                              array.m_dimensions.size()*sizeof(uint64_t), // Tile extent lengths in bytes
                              TILEDB_ROW_MAJOR, // Tile Order,
                              attribute_types));

  RETURN_ECANCELED_IF_ERROR(tiledb_array_create(TILEDB_CTX, &array_schema));
  RETURN_ECANCELED_IF_ERROR(tiledb_array_free_schema(&array_schema));

  return IMAGEDS_OK;
}
