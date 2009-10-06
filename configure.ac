# vi:set et ai sw=2 sts=2 ts=2: */
# -
# Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
# 
# This program is free software; you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of 
# the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public 
# License along with this program; if not, write to the Free 
# Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.

dnl ***************************
dnl *** Version information ***
dnl ***************************
m4_define([tumbler_verinfo], [0:0:0]) dnl TODO: Do we need to change this for the first release?
m4_define([tumbler_version_api_major], [1])
m4_define([tumbler_version_major], [0])
m4_define([tumbler_version_minor], [0])
m4_define([tumbler_version_micro], [0])
m4_define([tumbler_version_build], [r@REVISION@])
m4_define([tumbler_version_tag], [svn])
m4_define([tumbler_version], [tumbler_version_major().tumbler_version_minor().tumbler_version_micro()ifelse(tumbler_version_tag(), [svn], [tumbler_version_tag()-tumbler_version_build()], [tumbler_version_tag()])])

dnl ********************************************
dnl *** Full debug support for SVN snapshots ***
dnl ********************************************
m4_define([tumbler_debug_default], [ifelse(tumbler_version_tag(), [svn], [full], [minimum])])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>])
AC_INIT([tumbler], [tumbler_version], [http://bugzilla.xfce.org/])
AC_PREREQ([2.50])
AC_REVISION([@REVISION@])
AC_CANONICAL_TARGET()

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_AMFLAGS -I m4")
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()

dnl **************************
dnl *** Libtool versioning ***
dnl **************************
TUMBLER_VERINFO=tumbler_verinfo()
AC_SUBST([TUMBLER_VERINFO])

dnl ****************************
dnl *** Subst tumbler version ***
dnl ****************************
TUMBLER_VERSION_MAJOR=tumbler_version_major()
TUMBLER_VERSION_MINOR=tumbler_version_minor()
TUMBLER_VERSION_MICRO=tumbler_version_micro()
TUMBLER_VERSION_API_MAJOR=tumbler_version_api_major()
TUMBLER_VERSION_API=$TUMBLER_VERSION_API_MAJOR
AC_SUBST([TUMBLER_VERSION_MAJOR])
AC_SUBST([TUMBLER_VERSION_MINOR])
AC_SUBST([TUMBLER_VERSION_MICRO])
AC_SUBST([TUMBLER_VERSION_API_MAJOR])
AC_SUBST([TUMBLER_VERSION_API])

dnl *******************************
dnl *** Check for UNIX variants ***
dnl *******************************
AC_AIX()
AC_ISC_POSIX()
AC_MINIX()

dnl ********************************
dnl *** Check for Win32 variants ***
dnl ********************************
AC_MSG_CHECKING([if building for some Win32 platform])
case "$host" in
*-*-mingw*|*-*-cygwin*)
  ac_bm_platform_win32=yes
  ;;
*)
  ac_bm_platform_win32=no
  ;;
esac
AC_MSG_RESULT([$ac_bm_platform_win32])
AM_CONDITIONAL([PLATFORM_WIN32], [test x"$ac_bm_platform_win32" = x"yes"])

dnl ********************************
dnl *** Check for basic programs ***
dnl ********************************
AM_PROG_AS()
AC_PROG_CC()
AM_PROG_CC_C_O()
AC_PROG_INSTALL()
AC_PROG_INTLTOOL([0.31], [no-xml])
AC_PROG_LIBTOOL()

dnl ***************************************
dnl *** Check for standard header files ***
dnl ***************************************
AC_HEADER_STDC()
AC_CHECK_HEADERS([fcntl.h sys/mman.h sys/stat.h])

dnl ************************************
dnl *** Check for standard functions ***
dnl ************************************
AC_FUNC_MMAP()
#AC_CHECK_FUNCS([])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
linguas=`ls po/*.po | sed s/.po//g | sed sApo/AA | xargs`
XDT_I18N([$linguas])

dnl *************************
dnl *** Check for GTK-Doc ***
dnl *************************
GTK_DOC_CHECK(1.9)

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.16.0)
PKG_CHECK_MODULES(GIO, gio-2.0 >= 2.16.0)
PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= 2.16.0)
PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.0.0)
PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1 >= 0.72)

dnl ***************************
dnl *** Check PNG libraries ***
dnl ***************************
PKG_CHECK_MODULES(PNG, libpng >= 1.2.0, [have_libpng=yes], [
  dnl libpng.pc not found, try with libpng12.pc
  PKG_CHECK_MODULES(PNG, libpng12 >= 1.2.0, 
                    [have_libpng=yes],
                    [jhave_libpng=no])
])
AM_CONDITIONAL([HAVE_LIBPNG], test x"$have_libpng" = x"yes")

dnl *************************
dnl *** Check for plugins ***
dnl *************************
TUMBLER_PIXBUF_THUMBNAILER()
TUMBLER_FONT_THUMBNAILER()

dnl ************************************************************
dnl *** Check for thumbnail flavors (normal, large, cropped) ***
dnl ************************************************************
AC_ARG_ENABLE([normal-thumbnails], 
              [AC_HELP_STRING([--enable-normal-thumbnails], 
                              [Enable normal thumbnails (128 pixels) @<:@default=yes@:>@])],, 
              [enable_normal_thumbnails=yes])
if test x"$enable_normal_thumbnails" = x"yes"; then
  AC_DEFINE([ENABLE_NORMAL_THUMBNAILS], [1], [Define if built with support for normal thumbnails])
fi

AC_ARG_ENABLE([large-thumbnails], 
              [AC_HELP_STRING([--enable-large-thumbnails], 
                              [Enable large thumbnails (256 pixels) @<:@default=no@:>@])],, 
              [enable_large_thumbnails=no])
if test x"$enable_large_thumbnails" = x"yes"; then
  AC_DEFINE([ENABLE_LARGE_THUMBNAILS], [1], [Define if built with support for large thumbnails])
fi

AC_ARG_ENABLE([cropped-thumbnails], 
              [AC_HELP_STRING([--enable-cropped-thumbnails], 
                              [Enable cropped thumbnails (124 pixels) @<:@default=no@:>@])],, 
              [enable_cropped_thumbnails=no])
if test x"$enable_cropped_thumbnails" = x"yes"; then
  AC_DEFINE([ENABLE_CROPPED_THUMBNAILS], [1], [Define if built with support for cropped thumbnails])
fi

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG()

dnl **************************************
dnl *** Check for linker optimizations ***
dnl **************************************
XDT_FEATURE_LINKER_OPTS()

AC_OUTPUT([
Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/tumbler/Makefile
docs/reference/tumbler/version.xml
plugins/Makefile
plugins/font-thumbnailer/Makefile
plugins/pixbuf-thumbnailer/Makefile
plugins/xdg-cache/Makefile
po/Makefile.in
tumbler/Makefile
tumbler/tumbler-1.pc
tumbler/tumbler-config.h
tumblerd/Makefile
])

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "  * Debug:                            $enable_debug"
if test x"$ac_tumbler_pixbuf_thumbnailer" = x"yes"; then
echo "  * GdkPixbuf thumbnailer plugin:     yes"
else
echo "  * GdkPixbuf thumbnailer plugin:     no"
fi
if test x"$ac_tumbler_font_thumbnailer" = x"yes"; then
echo "  * FreeType font thumbnailer plugin: yes"
else
echo "  * FreeType font thumbnailer plugin: no"
fi
echo
echo "Supported Thumbnail Flavors:"
echo
if test x"$enable_normal_thumbnails" = x"yes"; then
echo "  * Normal (128 pixels):              yes"
else
echo "  * Normal (128 pixels):              no"
fi
if test x"$enable_large_thumbnails" = x"yes"; then
echo "  * Large (256 pixels):               yes"
else
echo "  * Large (256 pixels):               no"
fi
if test x"$enable_cropped_thumbnails" = x"yes"; then
echo "  * Cropped (124 pixels):             yes"
else
echo "  * Cropped (124 pixels):             no"
fi
echo