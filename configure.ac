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
m4_define([tumbler_verinfo], [0:0:0])
m4_define([tumbler_version_api_major], [1])
m4_define([tumbler_version_major], [0])
m4_define([tumbler_version_minor], [1])
m4_define([tumbler_version_micro], [1])
m4_define([tumbler_version], [tumbler_version_major().tumbler_version_minor().tumbler_version_micro()])

dnl ********************************************
dnl *** Full debug support for SVN snapshots ***
dnl ********************************************
m4_define([tumbler_debug_default], [minimum])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>])
AC_INIT([tumbler], [tumbler_version], [http://bugzilla.xfce.org/])
AC_PREREQ([2.50])
AC_CANONICAL_TARGET()

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([ACLOCAL_AMFLAGS], ["$ACLOCAL_FLAGS -I m4"])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

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
AC_CHECK_HEADERS([fcntl.h linux/sched.h memory.h sched.h setjmp.h stdio.h \
                  stdlib.h string.h syscall.h sys/mman.h sys/types.h \
                  sys/stat.h unistd.h])

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
dnl *** Check for dbus-binding-tool ***
dnl ***********************************
AC_ARG_VAR([DBUS_BINDING_TOOL], [Tool to generate C bindings from XML D-Bus interface definitions])
AC_PATH_PROG([DBUS_BINDING_TOOL], [dbus-binding-tool], [no])
if test x"$DBUS_BINDING_TOOL" = x"no"; then
  AC_MSG_ERROR([could not find dbus-binding-tool in \$PATH. You can run
  ./configure DBUS_BINDING_TOOL=/path/to/dbus-binding-tool to define 
  a custom location for it.])
fi

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.16.0])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.16.0])
PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= 2.16.0])
PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.0.0])
PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1 >= 0.72])

dnl *************************
dnl *** Check for plugins ***
dnl *************************
TUMBLER_FONT_THUMBNAILER()
TUMBLER_JPEG_THUMBNAILER()
TUMBLER_PIXBUF_THUMBNAILER()
TUMBLER_FFMPEG_THUMBNAILER()
TUMBLER_XDG_CACHE()

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG()

dnl **************************************
dnl *** Check for linker optimizations ***
dnl **************************************
XDT_FEATURE_LINKER_OPTS()

dnl *************************************
dnl *** Compensate for broken gtk-doc ***
dnl *************************************
XDT_SUPPORTED_FLAGS([GTK_DOC_EXTRA_CFLAGS], [-Wno-sign-compare])
AC_SUBST([GTK_DOC_EXTRA_CFLAGS])

AC_OUTPUT([
Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/tumbler/Makefile
docs/reference/tumbler/version.xml
plugins/Makefile
plugins/font-thumbnailer/Makefile
plugins/jpeg-thumbnailer/Makefile
plugins/pixbuf-thumbnailer/Makefile
plugins/ffmpeg-thumbnailer/Makefile
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
echo "  * Debug:                                     $enable_debug"
echo
echo "Plugins:"
echo
if test x"$ac_tumbler_pixbuf_thumbnailer" = x"yes"; then
echo "  * GdkPixbuf thumbnailer plugin:                     yes"
else
echo "  * GdkPixbuf thumbnailer plugin:                     no"
fi
if test x"$ac_tumbler_font_thumbnailer" = x"yes"; then
echo "  * FreeType font thumbnailer plugin:                 yes"
else
echo "  * FreeType font thumbnailer plugin:                 no"
fi
if test x"$ac_tumbler_jpeg_thumbnailer" = x"yes"; then
echo "  * JPEG thumbnailer plugin with EXIF support:        yes"
else
echo "  * JPEG thumbnailer plugin with EXIF support:        no"
fi
if test x"$ac_tumbler_ffmpeg_thumbnailer" = x"yes"; then
echo "  * Video thumbnailer plugin using ffmpegthumbnailer: yes"
else
echo "  * Video thumbnailer plugin using ffmpegthumbnailer: no"
fi
if test x"$ac_tumbler_xdg_cache" = x"yes"; then
echo "  * Freedesktop.org cache plugin:                     yes"
else
echo "  * Freedesktop.org cache plugin:                     no"
fi
echo
