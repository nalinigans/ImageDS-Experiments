/**
 * @file   error.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2019, Nalini Ganapati
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
 * @section DESCRIPTION
 *  Common Error Handling
 */

#ifndef IMAGEDS_ERROR_H
#define IMAGEDS_ERROR_H

#include <errno.h>
#include <exception>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

#define IMAGEDS_OK                                    0
#define IMAGEDS_ERR                                  -1

#define VERIFY(X) if(!(X)) throw ImageDSException(#X);

#define RETURN_IF_NULL(X) if (X == NULL) {errno = ECANCELED; return IMAGEDS_ERR;}

#define CHECK_ARG(X) if (X == NULL) {errno = EINVAL; return IMAGEDS_ERR;}
      
#define RETURN_ECONNREFUSED_IF_ERROR(...)       \
  do {                                          \
      int rc = __VA_ARGS__;                     \
      if (rc && !errno) errno = ECONNREFUSED;   \
      if (rc) return rc;                        \
      } while (false)                               

#define RETURN_EIO_IF_ERROR(...)                \
  do {                                          \
      int rc = __VA_ARGS__;                     \
      if (rc && !errno) errno = EIO;            \
      if (rc) return rc;                        \
      } while (false)

#define RETURN_EINVAL_IF_ERROR(...)             \
  do {                                          \
      int rc = __VA_ARGS__;                     \
      if (rc && !errno) errno = EINVAL;         \
      if (rc) return rc;                        \
      } while (false)

#define RETURN_ECANCELED_IF_ERROR(...)       \
  do {                                          \
      int rc = __VA_ARGS__;                     \
      if (rc && !errno) errno = ECANCELED;   \
      if (rc) return rc;                        \
      } while (false)

/**
 * ImageDSException is a catch all for all underlying ImageDS library exceptions.
 */
class ImageDSException : public std::exception {
 public:
  ImageDSException(const std::string m="ImageDS Exception: ") : m_msg(m) {}
  ~ImageDSException() {}
  /** Returns the exception message. */
  const char* what() const noexcept {
    return m_msg.c_str();
  }
 private:
  std::string m_msg;
};

      
#endif /* IMAGEDS_ERROR_H */
