dnl Process this file with autoconf to produce a configure script.

AC_INIT(configure.in)

AC_ARG_ENABLE(libpath)
if test "${enable_libpath}" = ""; then
  enable_libpath="/usr/lib"
fi

AC_DEFINE_UNQUOTED(CNMSLIBPATH, $enable_libpath/bjlib)
AC_SUBST(CNMSLIBPATH)

AC_SUBST(enable_libpath)

AM_INIT_AUTOMAKE(scangearmp, 1.60-1)
AM_CONFIG_HEADER(config.h)
AC_PROG_LIBTOOL
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_RANLIB

pkg_modules="gtk+-2.0 >= 2.6.0"
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

gimp_modules="gimp-2.0 >= 2.0.0"
PKG_CHECK_MODULES(GIMP, [$gimp_modules])
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)

dnl AC_CHECK_LIB(png, png_create_write_struct, PNG_LIBS="-lpng", PNG_LIBS="")
PNG_LIBS="-lpng"
AC_SUBST(PNG_LIBS)
AC_CHECK_HEADERS(png.h)

AC_CHECK_HEADERS(stdio.h)
AC_CHECK_HEADERS(libgimp/gimp.h)
AC_CHECK_HEADERS(libgimp/gimpcompat.h)

AC_CHECK_HEADERS(sane/sane.h)

AC_CHECK_LIB([usb], [usb_get_busses])
AC_CHECK_HEADERS(usb.h)

AC_CHECK_LIB([z], [deflate])
AC_CHECK_HEADERS(zlib.h)

GETTEXT_PACKAGE=scangearmp
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE")

dnl OPTION
CFLAGS="-O2 -D__GIMP_PLUGIN_ENABLE__ -D_FILE_OFFSET_BITS=64"

dnl Add the languages which your application supports here.
ALL_LINGUAS="cs da de el es fi fr hu id it ja ko nl nb pl pt ru sv th tr zh zh_TW"
AM_GLIB_GNU_GETTEXT

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${ac_default_prefix}/${DATADIRNAME}/locale")
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/${DATADIRNAME}/locale")
fi

AC_OUTPUT(
Makefile
po/Makefile.in
backend/Makefile
src/Makefile
include/Makefile
etc/Makefile)

