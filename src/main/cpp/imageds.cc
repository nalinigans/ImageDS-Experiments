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

#include "imageds.h"

#include "tiledb.h"
#include "tiledb_constants.h"
#include "tiledb_storage.h"
#include "tiledb_utils.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

#define TILEDB_CTX reinterpret_cast<TileDB_CTX*>(m_tiledb_ctx)

std::string imageds_version() {
  return IMAGEDS_VERSION;
}

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

int ImageDS::to_array(ImageDSArray& array, const std::vector<void *> buffers, const std::vector<size_t> buffer_sizes) {
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

  RETURN_EINVAL_IF_ERROR(tiledb_array_write(tiledb_array,
                                            const_cast<const void **>(buffers.data()),
                                            buffer_sizes.data()));

  //TODO: Check for overflow

  RETURN_ECANCELED_IF_ERROR(tiledb_array_finalize(tiledb_array));

  //TODO: Serialize TileDB_ArraySchema as JSON.
  //TileDB_ArraySchema schema;
  //tiledb_array_get_schema(tiledb_array, &schema);

  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_working_dir));
  return IMAGEDS_OK;
}

size_t dimensions_length(std::vector<ImageDSDimension> dimensions) {
  size_t size = 1;
  for (auto i=0ul; i<dimensions.size(); i++) {
    if (dimensions[i].m_end > dimensions[i].m_start) {
      size *= (dimensions[i].m_end-dimensions[i].m_start);
    } else {
      size *= (dimensions[i].m_start-dimensions[i].m_end);
    }
  }
  return size;
}

ImageDSBuffers ImageDS::create_read_buffers(ImageDSArray& array) {
  size_t required_length = 1;
  if (array.m_dimensions.size() == 0) {
    ImageDSArray array_from_schema;
    if (array_info(array.m_path, array_from_schema)) {
      throw std::runtime_error(std::string("Could not get array info from schema for ") + array.m_path);
    }
    required_length = dimensions_length(array_from_schema.m_dimensions);
  } else {
    required_length = dimensions_length(array.m_dimensions);
  }

  ImageDSBuffers imageds_buffers;
  std::vector<std::vector<uint8_t>> buffers;
  std::vector<size_t> buffer_sizes;
  for (auto i=0ul; i<array.m_attributes.size(); i++) {
    std::vector<uint8_t> buffer;
    switch (array.m_attributes[i].m_type) {
      case INT_8:
        break;
      case INT_32:
        required_length *= sizeof(int);
        break;
      case INT_64:
        required_length *= sizeof(long int);
        break;
      default:
        throw std::runtime_error("Not yet implemented!");
    }
    buffer.resize(required_length);
    imageds_buffers.add(buffer, required_length);
  }

  return imageds_buffers;
}

int ImageDS::from_array(ImageDSArray& array, std::vector<void *> buffers, std::vector<size_t> buffer_size) {
  RETURN_EIO_IF_ERROR(set_working_dir(TILEDB_CTX, m_workspace));

  std::vector<char *> attributes;
  for (auto i=0ul; i<array.m_attributes.size(); i++) {
    attributes.push_back(const_cast<char *>(array.m_attributes[i].m_name.c_str()));
  }

  const char **tiledb_attributes;
  int attribute_num;
  if (attributes.empty()) {
    tiledb_attributes = NULL; // All attributes
    attribute_num = 0;
  } else {
    tiledb_attributes = const_cast<const char**>(attributes.data());
    attribute_num = attributes.size();
  }
  
  TileDB_Array* tiledb_array;
  size_t dimensions_length = array.m_dimensions.size();
  if (dimensions_length > 0) {
    uint64_t subarray[dimensions_length*2];
    for (auto i=0ul; i<dimensions_length; i++) {
      subarray[i*2] = array.m_dimensions[i].m_start;
      subarray[i*2+1] = array.m_dimensions[i].m_end;
    }
    RETURN_EINVAL_IF_ERROR(tiledb_array_init(TILEDB_CTX, &tiledb_array,
                                             array.m_path.c_str(),
                                             TILEDB_ARRAY_READ_SORTED_ROW,
                                             subarray, 
                                             tiledb_attributes,
                                             attribute_num));
  } else {
    RETURN_EINVAL_IF_ERROR(tiledb_array_init(TILEDB_CTX, &tiledb_array,
                                             array.m_path.c_str(),
                                             TILEDB_ARRAY_READ_SORTED_ROW,
                                             NULL, // Entire Domain
                                             tiledb_attributes,
                                             attribute_num));
  }
  
  RETURN_ECANCELED_IF_ERROR(tiledb_array_read(tiledb_array,
                                              buffers.data(),
                                              buffer_size.data()));
  
  for (auto i=0ul; i<array.m_attributes.size(); i++) {
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
