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
dnl Check whether to build and install the GdkPibuxf thumbnailer
dnl
AC_DEFUN([TUMBLER_PIXBUF_THUMBNAILER],
[
AC_ARG_ENABLE([pixbuf-thumbnailer], [AC_HELP_STRING([--disable-pixbuf-thumbnailer], [Don't build the GdkPixbuf thumbnailer plugin])],
  [ac_tumbler_pixbuf_thumbnailer=$enableval], [ac_tumbler_pixbuf_thumbnailer=yes])
if test x"$ac_tumbler_pixbuf_thumbnailer" = x"yes"; then
  PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= 2.14], [], [ac_tumbler_pixbuf_thumbnailer=no])
fi

AC_MSG_CHECKING([whether to build the GdkPixbuf thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_PIXBUF_THUMBNAILER], [test x"$ac_tumbler_pixbuf_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_pixbuf_thumbnailer])
])



dnl TUMBLER_FONT_THUMBNAILER()
dnl
dnl Check whether to build and install the GdkPibuxf thumbnailer
dnl
AC_DEFUN([TUMBLER_FONT_THUMBNAILER],
[
AC_ARG_ENABLE([font-thumbnailer], [AC_HELP_STRING([--disable-font-thumbnailer], [Don't build the FreeType font thumbnailer plugin])],
  [ac_tumbler_font_thumbnailer=$enableval], [ac_tumbler_font_thumbnailer=yes])
if test x"$ac_tumbler_font_thumbnailer" = x"yes"; then
  dnl Check for FreeType 2.x
  FREETYPE_LIBS=""
  FREETYPE_CFLAGS=""
  AC_PATH_PROG([FREETYPE_CONFIG], [freetype-config], [no])
  if test x"$FREETYPE_CONFIG" != x"no"; then
    AC_MSG_CHECKING([FREETYPE_CFLAGS])
    FREETYPE_CFLAGS="`$FREETYPE_CONFIG --cflags`"
    AC_MSG_RESULT([$FREETYPE_CFLAGS])
  
    AC_MSG_CHECKING([FREETYPE_LIBS])
    FREETYPE_LIBS="`$FREETYPE_CONFIG --libs`"
    AC_MSG_RESULT([$FREETYPE_LIBS])
  else
    dnl We can only build the font thumbnailer if FreeType 2.x is available
    ac_tumbler_font_thumbnailer=no
  fi
  AC_SUBST([FREETYPE_CFLAGS])
  AC_SUBST([FREETYPE_LIBS])
fi

AC_MSG_CHECKING([whether to build the FreeType thumbnailer plugin])
AM_CONDITIONAL([TUMBLER_FONT_THUMBNAILER], [test x"$ac_tumbler_font_thumbnailer" = x"yes"])
AC_MSG_RESULT([$ac_tumbler_font_thumbnailer])
])
