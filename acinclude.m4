dnl vi:set et ai sw=2 sts=2 ts=2: */
dnl -
dnl Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
dnl
dnl This program is free software; you can redistribute it and/or 
dnl modify it under the terms of the GNU General Public License as
dnl published by the Free Software Foundation; either version 2 of 
dnl the License, or (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public 
dnl License along with this program; if not, write to the Free 
dnl Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
dnl Boston, MA 02110-1301, USA.



dnl TUMBLER_PIXBUF_THUMBNAILER()
dnl
dnl Check whether to build and install the GdkPibuxf thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_PIXBUF_THUMBNAILER],
[
AC_ARG_ENABLE([pixbuf-thumbnailer], [AS_HELP_STRING([--disable-pixbuf-thumbnailer], [Don't build the GdkPixbuf thumbnailer plugin])],
  [ac_tumbler_pixbuf_thumbnailer=$enableval], [ac_tumbler_pixbuf_thumbnailer=yes])
if test x"$ac_tumbler_pixbuf_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0], [], [ac_tumbler_pixbuf_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the GdkPixbuf thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_PIXBUF_THUMBNAILER], [test x"$ac_tumbler_pixbuf_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_pixbuf_thumbnailer])
])



dnl TUMBLER_FONT_THUMBNAILER()
dnl
dnl Check whether to build and install the FreeType2 font thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_FONT_THUMBNAILER],
[
AC_ARG_ENABLE([font-thumbnailer], [AS_HELP_STRING([--disable-font-thumbnailer], [Don't build the FreeType font thumbnailer plugin])],
  [ac_tumbler_font_thumbnailer=$enableval], [ac_tumbler_font_thumbnailer=yes])
if test x"$ac_tumbler_font_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf 
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for FreeType 2.x
    PKG_CHECK_MODULES([FREETYPE], [freetype2], [], [ac_tumbler_font_thumbnailer=no])
  ], [ac_tumbler_font_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the FreeType thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_FONT_THUMBNAILER], [test x"$ac_tumbler_font_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_font_thumbnailer])
])



dnl TUMBLER_JPEG_THUMBNAILER()
dnl
dnl Check whether to build and install the JPEG thumbnailer plugin with 
dnl EXIF support.
dnl
AC_DEFUN([TUMBLER_JPEG_THUMBNAILER],
[
AC_ARG_ENABLE([jpeg-thumbnailer], [AS_HELP_STRING([--disable-jpeg-thumbnailer], [Don't build the JPEG thumbnailer plugin with EXIF support])],
  [ac_tumbler_jpeg_thumbnailer=$enableval], [ac_tumbler_jpeg_thumbnailer=yes])
if test x"$ac_tumbler_jpeg_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf 
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for libjpeg
    LIBJPEG_LIBS=""
    LIBJPEG_CFLAGS=""
    AC_CHECK_LIB([jpeg], [jpeg_start_decompress],
    [
      AC_CHECK_HEADER([jpeglib.h],
      [
        LIBJPEG_LIBS="-ljpeg -lm"
      ],
      [
        dnl We can only build the JPEG thumbnailer if the JPEG headers are available
        ac_tumbler_jpeg_thumbnailer=no
      ])
    ],
    [
      dnl We can only build the JPEG thumbnailer if libjpeg is available
      ac_tumbler_jpeg_thumbnailer=no
    ])
    AC_SUBST([LIBJPEG_CFLAGS])
    AC_SUBST([LIBJPEG_LIBS])
  ], [ac_tumbler_jpeg_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the JPEG thumbnailer plugin with EXIF support])
AM_CONDITIONAL([TUMBLER_JPEG_THUMBNAILER], [test x"$ac_tumbler_jpeg_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_jpeg_thumbnailer])
])


dnl TUMBLER_FFMPEG_THUMBNAILER()
dnl
dnl Check whether to build and install the ffmpeg video thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_FFMPEG_THUMBNAILER],
[
AC_ARG_ENABLE([ffmpeg-thumbnailer], [AS_HELP_STRING([--disable-ffmpeg-thumbnailer], [Don't build the ffmpeg video thumbnailer plugin])],
  [ac_tumbler_ffmpeg_thumbnailer=$enableval], [ac_tumbler_ffmpeg_thumbnailer=yes])
if test x"$ac_tumbler_ffmpeg_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for libffmpegthumbnailer
    FFMPEG_REQUIRED_VERSION="2.0.0"
    PKG_CHECK_MODULES([FFMPEGTHUMBNAILER], [libffmpegthumbnailer >= $FFMPEG_REQUIRED_VERSION], [
      dnl To build our own CHECK_VERSION, not provided at least until 2.2.2
      FFMPEG_VERSION=$($PKG_CONFIG --modversion libffmpegthumbnailer)
      echo "$FFMPEG_VERSION" | $EGREP ['^[0-9]+\.[0-9]+\.[0-9]+$'] >/dev/null || FFMPEG_VERSION=$FFMPEG_REQUIRED_VERSION
      FFMPEG_MAJOR_VERSION=${FFMPEG_VERSION%%.*}
      FFMPEG_MICRO_VERSION=${FFMPEG_VERSION##*.}
      FFMPEG_MINOR_VERSION=${FFMPEG_VERSION#*.}
      FFMPEG_MINOR_VERSION=${FFMPEG_MINOR_VERSION%.*}
      AC_DEFINE_UNQUOTED([TUMBLER_FFMPEG_MAJOR_VERSION], [$FFMPEG_MAJOR_VERSION], [For libffmpegthumbnailer version check])
      AC_DEFINE_UNQUOTED([TUMBLER_FFMPEG_MINOR_VERSION], [$FFMPEG_MINOR_VERSION], [For libffmpegthumbnailer version check])
      AC_DEFINE_UNQUOTED([TUMBLER_FFMPEG_MICRO_VERSION], [$FFMPEG_MICRO_VERSION], [For libffmpegthumbnailer version check])
    ], [ac_tumbler_ffmpeg_thumbnailer=no])
  ], [ac_tumbler_ffmpeg_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the ffmpeg video thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_FFMPEG_THUMBNAILER], [test x"$ac_tumbler_ffmpeg_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_ffmpeg_thumbnailer])
])



dnl TUMBLER_GSTREAMER_THUMBNAILER()
dnl
dnl Check whether to build and install the gstreamer video thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_GSTREAMER_THUMBNAILER],
[
AC_ARG_ENABLE([gstreamer-thumbnailer], [AS_HELP_STRING([--disable-gstreamer-thumbnailer], [Don't build the GStreamer video thumbnailer plugin])],
  [ac_tumbler_gstreamer_thumbnailer=$enableval], [ac_tumbler_gstreamer_thumbnailer=yes])
if test x"$ac_tumbler_gstreamer_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for libgstreamer
    PKG_CHECK_MODULES([GSTREAMER], [gstreamer-1.0], [
      dnl Check for libgstreamertag
      PKG_CHECK_MODULES([GSTREAMER_TAG], [gstreamer-tag-1.0], [], [ac_tumbler_gstreamer_thumbnailer=no])
    ], [ac_tumbler_gstreamer_thumbnailer=no])
  ], [ac_tumbler_gstreamer_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the gstreamer video thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_GSTREAMER_THUMBNAILER], [test x"$ac_tumbler_gstreamer_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_gstreamer_thumbnailer])
])



dnl TUMBLER_POPPLER_THUMBNAILER()
dnl
dnl Check whether to build and install the poppler PDF/PS thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_POPPLER_THUMBNAILER],
[
AC_ARG_ENABLE([poppler-thumbnailer], [AS_HELP_STRING([--disable-poppler-thumbnailer], [Don't build the poppler PDF/PS thumbnailer plugin])],
  [ac_tumbler_poppler_thumbnailer=$enableval], [ac_tumbler_poppler_thumbnailer=yes])
if test x"$ac_tumbler_poppler_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for poppler-glib
    PKG_CHECK_MODULES([POPPLER_GLIB], [poppler-glib >= 0.12.0], [], [ac_tumbler_poppler_thumbnailer=no])
  ], [ac_tumbler_poppler_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the poppler PDF/PS thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_POPPLER_THUMBNAILER], [test x"$ac_tumbler_poppler_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_poppler_thumbnailer])
])



dnl TUMBLER_ODF_THUMBNAILER()
dnl
dnl Check whether to build and install the ODF thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_ODF_THUMBNAILER],
[
AC_ARG_ENABLE([odf-thumbnailer], [AS_HELP_STRING([--disable-odf-thumbnailer], [Don't build the office thumbnailer plugin])],
  [ac_tumbler_odf_thumbnailer=$enableval], [ac_tumbler_odf_thumbnailer=yes])
if test x"$ac_tumbler_odf_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for libgsf
    PKG_CHECK_MODULES([GSF], [libgsf-1 >= 1.14.9], [], [ac_tumbler_odf_thumbnailer=no])
  ], [ac_tumbler_odf_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the office thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_ODF_THUMBNAILER], [test x"$ac_tumbler_odf_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_odf_thumbnailer])
])



dnl TUMBLER_RAW_THUMBNAILER()
dnl
dnl Check whether to build and install the libopenraw thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_RAW_THUMBNAILER],
[
AC_ARG_ENABLE([raw-thumbnailer], [AS_HELP_STRING([--disable-raw-thumbnailer], [Don't build the RAW image thumbnailer plugin])],
  [ac_tumbler_raw_thumbnailer=$enableval], [ac_tumbler_raw_thumbnailer=yes])
if test x"$ac_tumbler_raw_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for libopenraw
    dnl From most recent to oldest, hopefully from most likely to least likely
    PKG_CHECK_MODULES([LIBOPENRAW_GNOME], [libopenraw-gnome-0.3 >= 0.0.4], [],
    [
      PKG_CHECK_MODULES([LIBOPENRAW_GNOME], [libopenraw-gnome-0.2 >= 0.0.4], [],
      [
        dnl Note: 0.1.0 release changed the pkg-config name from -1.0 to -0.1
        PKG_CHECK_MODULES([LIBOPENRAW_GNOME], [libopenraw-gnome-0.1 >= 0.0.4], [],
        [
          PKG_CHECK_MODULES([LIBOPENRAW_GNOME], [libopenraw-gnome-1.0 >= 0.0.4], [], [ac_tumbler_raw_thumbnailer=no])
        ])
      ])
    ])
  ], [ac_tumbler_raw_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the RAW image thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_RAW_THUMBNAILER], [test x"$ac_tumbler_raw_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_raw_thumbnailer])
])



dnl TUMBLER_COVER_THUMBNAILER()
dnl
dnl Check whether to build and install the Open Movie Database thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_COVER_THUMBNAILER],
[
AC_ARG_ENABLE([cover-thumbnailer], [AS_HELP_STRING([--disable-cover-thumbnailer], [Don't build the Cover thumbnailer plugin])],
  [ac_tumbler_cover_thumbnailer=$enableval], [ac_tumbler_cover_thumbnailer=yes])
if test x"$ac_tumbler_cover_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for curl
    PKG_CHECK_MODULES([CURL], [libcurl], [7.32.0], [ac_tumbler_cover_thumbnailer=no])
  ], [ac_tumbler_cover_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the Cover thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_COVER_THUMBNAILER], [test x"$ac_tumbler_cover_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_cover_thumbnailer])
])



dnl TUMBLER_XDG_CACHE()
dnl
dnl Check whether to build and install the freedesktop.org cache plugin.
dnl
AC_DEFUN([TUMBLER_XDG_CACHE],
[
AC_ARG_ENABLE([xdg-cache], [AS_HELP_STRING([--disable-xdg-cache], [Don't build the freedesktop.org cache plugin])],
  [ac_tumbler_xdg_cache=$enableval], [ac_tumbler_xdg_cache=yes])
if test x"$ac_tumbler_xdg_cache" = x"yes"; then
  dnl Check for gdk-pixbuf 
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for PNG libraries
    PKG_CHECK_MODULES(PNG, libpng >= 1.2.0, [have_libpng=yes], 
    [
      dnl libpng.pc not found, try with libpng12.pc
      PKG_CHECK_MODULES(PNG, libpng12 >= 1.2.0, [have_libpng=yes], 
      [
        have_libpng=no
        ac_tumbler_xdg_cache=no
      ])
    ])
  ], [ac_tumbler_xdg_cache=no])
fi

AC_MSG_CHECKING([whether to build the freedesktop.org cache plugin])
AM_CONDITIONAL([TUMBLER_XDG_CACHE], [test x"$ac_tumbler_xdg_cache" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_xdg_cache])
])

dnl TUMBLER_DESKTOP_THUMBNAILER()
dnl
dnl Check whether to build and install the thumbnailers plugin support for loading thumbnailers *.thumbnailer files
dnl
AC_DEFUN([TUMBLER_DESKTOP_THUMBNAILER],
[
AC_ARG_ENABLE([desktop-thumbnailer], [AS_HELP_STRING([--disable-desktop-thumbnailer], [Don't build the plugin support for loading thumbnailers *.thumbnailer files])],
  [ac_tumbler_desktop_thumbnailer=$enableval], [ac_tumbler_desktop_thumbnailer=yes])
if test x"$ac_tumbler_desktop_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0], [], [ac_tumbler_desktop_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the plugin support for loading thumbnailers *.thumbnailer files])
AM_CONDITIONAL([TUMBLER_DESKTOP_THUMBNAILER], [test x"$ac_tumbler_desktop_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_desktop_thumbnailer])
])

dnl TUMBLER_GEPUB_THUMBNAILER()
dnl
dnl Check whether to build the libgepub thumbnailer plugin.
dnl
AC_DEFUN([TUMBLER_GEPUB_THUMBNAILER],
[
AC_ARG_ENABLE([gepub-thumbnailer], [AS_HELP_STRING([--disable-gepub-thumbnailer], [Don't build the libgepub thumbnailer plugin])],
  [ac_tumbler_gepub_thumbnailer=$enableval], [ac_tumbler_gepub_thumbnailer=yes])
if test x"$ac_tumbler_gepub_thumbnailer" = x"yes"; then
  dnl Check for gdk-pixbuf
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.40.0],
  [
    dnl Check for libgepub
    PKG_CHECK_MODULES([GEPUB], [libgepub-0.7 >= 0.7.0], [], [
      PKG_CHECK_MODULES([GEPUB], [libgepub-0.6 >= 0.6.0], [], [ac_tumbler_gepub_thumbnailer=no])
    ])
  ], [ac_tumbler_gepub_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the libgepub thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_GEPUB_THUMBNAILER], [test x"$ac_tumbler_gepub_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_gepub_thumbnailer])
])
