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

/* \file utils.cpp
   \brief utility routines for maths operations, text operations,
   I/O conversions, etc
*/
#include "utils.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

using std::string;
using std::vector;

Status read_double(const char *str, double *f)
{
  bool to_sqrt;
  char buff;
  if (sscanf(str, " sqrt%lf %c", f, &buff) == 1)
    to_sqrt = true;
  else if (sscanf(str, " %lf %c", f, &buff) == 1)
    to_sqrt = false;
  else
    return Status::error("not a number");

  if (isinf(*f))
    return Status::error("number too large\n");

  if (isnan(*f))
    return Status::error("not a number\n");

  if (to_sqrt)
    *f = sqrt(*f);

  return Status::ok();
}

Status read_int(const char *str, int *i)
{
  char buff;
  if (sscanf(str, " %d %c", i, &buff) != 1)
    return Status::error("not an integer");

  if (*i == INT_MAX)
    return Status::error("integer too large\n");

  return Status::ok();
}

// https://stackoverflow.com/questions/2342162/stdstring-formatting-
// like-sprintf/49812018#49812018
string msg_str(const char *fmt, ...)
{
  // initialize use of the variable argument array
  va_list ap;
  va_start(ap, fmt);

  // reliably acquire the size
  // from a copy of the variable argument array
  // and a functionally reliable call to mock the formatting
  va_list ap_copy;
  va_copy(ap_copy, ap);
  const int len = vsnprintf(NULL, 0, fmt, ap_copy);
  va_end(ap_copy);

  // return a formatted string without risking memory mismanagement
  // and without assuming any compiler or platform specific behavior
  vector<char> vstr(len + 1);
  vsnprintf(vstr.data(), vstr.size(), fmt, ap);
  va_end(ap);
  return string(vstr.data(), len);
}
