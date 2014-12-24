#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([g60cuse], [0.10])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([1.11 parallel-tests])
AC_GNU_SOURCE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE(
	[debug],
	[AS_HELP_STRING([--enable-debug],[enable debugging code and output [default=no]])],
	[enable_debug=$enableval],
	[enable_debug="no"]
)

if test "x$enable_debug" = xyes; then
	CFLAGS="$CFLAGS -O0 -ggdb -fno-inline"
fi

AS_COMPILER_FLAGS(WFLAGS,
        "-Wall
         -Wextra
         -Wno-missing-field-initializers
         -Wno-sign-compare
         -Wno-pointer-sign
         -Wno-unused-parameter
         -Werror=pointer-to-int-cast
         -Wdeclaration-after-statement
         -Werror-implicit-function-declaration
         -Wformat-security
         -Winit-self
         -Wno-missing-declarations
         -Wmissing-include-dirs
         -Wnested-externs
         -Wpointer-arith
         -Wwrite-strings")
AC_SUBST(WFLAGS, [$WFLAGS])

# Checks for programs.
AC_PROG_CC
AC_CONFIG_FILES([Makefile])

# Checks for libraries.
PKG_CHECK_MODULES([FUSE], [fuse])
PKG_CHECK_MODULES([USB], [libusb])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset])

AC_OUTPUT
