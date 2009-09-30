# vi:set ts=8 sw=8 noet ai nocindent:
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

if ENABLE_FONT_THUMBNAILER

tumbler_plugindir = $(libdir)/tumbler-$(TUMBLER_VERSION_API)/plugins
tumbler_plugin_LTLIBRARIES =						\
	tumbler-font-thumbnailer.la

tumbler_font_thumbnailer_la_SOURCES =					\
	font-thumbnailer-plugin.c					\
	font-thumbnailer-provider.c					\
	font-thumbnailer-provider.h					\
	font-thumbnailer.c						\
	font-thumbnailer.h

tumbler_font_thumbnailer_la_CFLAGS =					\
	-I$(top_builddir)						\
	-I$(top_builddir)/plugins					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/plugins						\
	-DG_LOG_DOMAIN=\"tumbler-font-thumbnailer\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(FREETYPE_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

tumbler_font_thumbnailer_la_LDFLAGS =					\
	-avoid-version							\
	-export-dynamic							\
	-module								\
	$(PLATFORM_LDFLAGS)

tumbler_font_thumbnailer_la_LIBADD =					\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(FREETYPE_LIBS)

endif
