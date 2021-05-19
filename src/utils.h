/*
   Copyright (c) 2003-2018, Adrian Rossiter

   Antiprism - http://www.antiprism.com

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be included
      in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.
*/

/*!\file utils.h
   \brief utility routines for text operations, I/O conversions, etc
*/

#ifndef UTILS_H
#define UTILS_H

#include "status_msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

/// Read a floating point number, or mathematical expression, from a string.
/** The string should only hold the floating point number or expression, but may
 *  have leading and trailing whitespace.
 * \param str the string holding the floating point number or expressions.
 * \param f used to return the floating point number.
 * \return status, evaluates to \c true if a valid floating point number
 *  was read, otherwise \c false.*/
Status read_double(const char *str, double *f);

/// Read an integer from a string.
/** The string should only hold the integer, but may
 *  have leading and trailing whitespace.
 * \param str the string holding the integer.
 * \param i used to return the integer.
 * \return status, evaluates to \c true if a valid integer
 *  was read, otherwise \c false.*/
Status read_int(const char *str, int *i);

/// Join strings into a single string, with parts separated by a delimiter
/**\param iterator to first string in sequence
 * \param iterator to end of string sequence
 * \param delim delimiter to include between strings
 * \return The joined and delimited strings */
template <typename InputIt>
std::string join(InputIt first, InputIt last, std::string delim = " ")
{
  std::string str;
  for (; first != last; ++first)
    str += *first + ((std::next(first) != last) ? delim : std::string());
  return str;
}

/// Convert a C formated message string to a C++ string
/** Converts the first MSG_SZ-1 characters of the C format string
 * \param fmt the formatted string
 * \param ... the values for the format
 * \return The converted string. */
std::string msg_str(const char *fmt, ...);

#endif // UTILS_H
