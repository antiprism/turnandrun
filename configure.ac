
AC_PREREQ([2.69])
AC_INIT([turnandrun], [0.02], [adrian@antiprism.com])
AM_INIT_AUTOMAKE([1.9 foreign])
LT_INIT

AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Set compiler
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX([14], [noext])

inst_msg="Install a development version of this library and rerun ./configure"
AC_LANG_PUSH([C++])

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([gettimeofday sqrt strchr strcspn])


# Checks for libraries.
AC_CHECK_LIB([iio], [iio_create_default_context], [], [AC_MSG_ERROR([$inst_msg])])
AC_CHECK_HEADER([iio.h], [], [AC_MSG_ERROR([$inst_msg])])

AC_LANG_POP([C++])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 scripts/turnandrun_service_install
                 ])

AC_OUTPUT
