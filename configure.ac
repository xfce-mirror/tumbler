# vi:set et ai sw=2 sts=2 ts=2: */
# -
# Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
# Copyright (c) 2015 The Xfce Development Team
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
XDT_VERSION_INIT([4.18.0], [git])

dnl ***************************
dnl *** Initialize autoconf ***
dnl ***************************
AC_COPYRIGHT([Copyright (c) 2018-2023 The Xfce development team. All rights reserved.
  Written for Xfce by Jannis Pohlmann <jannis@xfce.org>.])
AC_INIT([tumbler], [xdt_version], [https://gitlab.xfce.org/xfce/tumbler])
AC_PREREQ([2.69])
AC_REVISION([xdt_version_build])

dnl ***************************
dnl *** Initialize automake ***
dnl ***************************
AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar no-dist-gzip foreign])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS([config.h])
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
TUMBLER_VERSION_MAJOR=xdt_version_major()
TUMBLER_VERSION_MINOR=xdt_version_minor()
TUMBLER_VERSION_MICRO=xdt_version_micro()
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
AC_USE_SYSTEM_EXTENSIONS
AC_SEARCH_LIBS([strerror],[cposix])

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
AC_PROG_GREP()
AC_PROG_EGREP()
AC_PROG_SED()

dnl ***********************
dnl *** Prepare libtool ***
dnl ***********************
LT_PREREQ([2.4])
LT_INIT([disable-static])

dnl ***************************************
dnl *** Check for standard header files ***
dnl ***************************************
AC_CHECK_HEADERS([fcntl.h linux/sched.h memory.h sched.h setjmp.h stdio.h \
                  stdlib.h string.h syscall.h sys/mman.h sys/types.h \
                  sys/stat.h unistd.h sys/select.h pwd.h math.h])
AC_SEARCH_LIBS([round], [m], [], [AC_MSG_ERROR([unable to find the round() function in math library])])

dnl ************************************
dnl *** Check for standard functions ***
dnl ************************************
AC_FUNC_MMAP()
AC_CHECK_FUNCS([sched_getparam sched_setscheduler getpwnam bind_textdomain_codeset])

dnl ******************************
dnl *** Check for i18n support ***
dnl ******************************
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl *************************
dnl *** Check for GTK-Doc ***
dnl *************************
GTK_DOC_CHECK(1.9)

dnl ***********************************
dnl *** Check for gdbus-codegen ***
dnl ***********************************
XDT_CHECK_PACKAGE_BINARY([GDBUS_CODEGEN], [gio-2.0], [gdbus_codegen], [gdbus-codegen])

dnl ***********************************
dnl *** Check for required packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.66.0])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [2.66.0])
XDT_CHECK_PACKAGE([GIO_UNIX], [gio-unix-2.0], [2.66.0])
XDT_CHECK_PACKAGE([GMODULE], [gmodule-2.0], [2.66.0])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.66.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.17.1])

dnl ******************************************************************
dnl *** Alternative service name prefix for e.g. Flatpak packaging ***
dnl ******************************************************************
AC_ARG_VAR([TUMBLER_SERVICE_NAME_PREFIX],
           [Alternative prefix to org.freedesktop.thumbnails for Tumbler services])
if test -z "$TUMBLER_SERVICE_NAME_PREFIX"; then
  TUMBLER_SERVICE_NAME_PREFIX='org.freedesktop.thumbnails'
  AC_SUBST([TUMBLER_SERVICE_FILENAME_PREFIX], [org.xfce.Tumbler])
else
  AC_SUBST([TUMBLER_SERVICE_FILENAME_PREFIX], [$TUMBLER_SERVICE_NAME_PREFIX])
fi

AC_SUBST([TUMBLER_SERVICE_PATH_PREFIX],
         [$(printf '%s' $TUMBLER_SERVICE_NAME_PREFIX | $SED -E 's%(^|\.)%/%g')])
AC_DEFINE_UNQUOTED(TUMBLER_SERVICE_NAME_PREFIX, "$TUMBLER_SERVICE_NAME_PREFIX",
                   Name prefix for Tumbler services)
AC_DEFINE_UNQUOTED(TUMBLER_SERVICE_PATH_PREFIX, "$TUMBLER_SERVICE_PATH_PREFIX",
                   Path prefix for Tumbler services)

dnl *************************
dnl *** Check for plugins ***
dnl *************************
TUMBLER_COVER_THUMBNAILER()
TUMBLER_FONT_THUMBNAILER()
TUMBLER_JPEG_THUMBNAILER()
TUMBLER_PIXBUF_THUMBNAILER()
TUMBLER_FFMPEG_THUMBNAILER()
TUMBLER_GSTREAMER_THUMBNAILER()
TUMBLER_ODF_THUMBNAILER()
TUMBLER_POPPLER_THUMBNAILER()
TUMBLER_RAW_THUMBNAILER()
TUMBLER_XDG_CACHE()
TUMBLER_DESKTOP_THUMBNAILER()
TUMBLER_GEPUB_THUMBNAILER()

dnl ***********************************
dnl *** Check for debugging support ***
dnl ***********************************
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl **************************************
dnl *** Check for linker optimizations ***
dnl **************************************
XDT_FEATURE_LINKER_OPTS()

dnl *************************************
dnl *** Compensate for broken gtk-doc ***
dnl *************************************
XDT_SUPPORTED_FLAGS([GTK_DOC_EXTRA_CFLAGS], [-Wno-sign-compare])
AC_SUBST([GTK_DOC_EXTRA_CFLAGS])

AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/tumbler/Makefile
docs/reference/tumbler/version.xml
icons/Makefile
icons/32x32/Makefile
plugins/Makefile
plugins/cover-thumbnailer/Makefile
plugins/font-thumbnailer/Makefile
plugins/gst-thumbnailer/Makefile
plugins/jpeg-thumbnailer/Makefile
plugins/pixbuf-thumbnailer/Makefile
plugins/ffmpeg-thumbnailer/Makefile
plugins/odf-thumbnailer/Makefile
plugins/poppler-thumbnailer/Makefile
plugins/raw-thumbnailer/Makefile
plugins/xdg-cache/Makefile
plugins/desktop-thumbnailer/Makefile
plugins/gepub-thumbnailer/Makefile
po/Makefile.in
tumbler/Makefile
tumbler/tumbler-1.pc
tumbler/tumbler-config.h
tumblerd/Makefile
])
AC_OUTPUT

dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "Build Configuration:"
echo
echo "  * Debug:                                            $enable_debug"
echo
echo "Plugins:"
echo
if test x"$ac_tumbler_pixbuf_thumbnailer" = x"yes"; then
echo "  * GdkPixbuf thumbnailer plugin:                     yes"
else
echo "  * GdkPixbuf thumbnailer plugin:                     no"
fi
if test x"$ac_tumbler_cover_thumbnailer" = x"yes"; then
echo "  * Cover thumbnailer plugin:                         yes"
else
echo "  * Cover thumbnailer plugin:                         no"
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
if test x"$ac_tumbler_gstreamer_thumbnailer" = x"yes"; then
echo "  * Video thumbnailer plugin using GStreamer:         yes"
else
echo "  * Video thumbnailer plugin using GStreamer:         no"
fi
if test x"$ac_tumbler_odf_thumbnailer" = x"yes"; then
echo "  * ODF thumbnailer plugin using libgsf:              yes"
else
echo "  * ODF thumbnailer plugin using libgsf:              no"
fi
if test x"$ac_tumbler_poppler_thumbnailer" = x"yes"; then
echo "  * PDF/PS thumbnailer plugin using poppler:          yes"
else
echo "  * PDF/PS thumbnailer plugin using poppler:          no"
fi
if test x"$ac_tumbler_raw_thumbnailer" = x"yes"; then
echo "  * RAW thumbnailer plugin using libopenraw:          yes"
else
echo "  * RAW thumbnailer plugin using libopenraw:          no"
fi
if test x"$ac_tumbler_xdg_cache" = x"yes"; then
echo "  * Freedesktop.org cache plugin:                     yes"
else
echo "  * Freedesktop.org cache plugin:                     no"
fi
if test x"$ac_tumbler_desktop_thumbnailer" = x"yes"; then
echo "  * Loading thumbnailers from .thumbnailer files:     yes"
else
echo "  * Loading thumbnailers from .thumbnailer files:     no"
fi
if test x"$ac_tumbler_gepub_thumbnailer" = x"yes"; then
echo "  * EPUB thumbnailer using libgepub:                  yes"
else
echo "  * EPUB thumbnailer using libgepub:                  no"
fi
echo
