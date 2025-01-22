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

if TUMBLER_XDG_CACHE

tumbler_plugindir = $(libdir)/tumbler-$(TUMBLER_VERSION_API)/plugins/cache
tumbler_plugin_LTLIBRARIES =						\
	tumbler-xdg-cache.la

tumbler_xdg_cache_la_SOURCES =						\
	xdg-cache-cache.c						\
	xdg-cache-cache.h						\
	xdg-cache-plugin.c						\
	xdg-cache-thumbnail.c						\
	xdg-cache-thumbnail.h

tumbler_xdg_cache_la_CFLAGS =						\
	-I$(top_builddir)						\
	-I$(top_builddir)/plugins					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/plugins						\
	-DG_LOG_DOMAIN=\"tumbler-xdg-cache\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS)                  \
	$(PNG_CFLAGS)

tumbler_xdg_cache_la_LDFLAGS =						\
	-avoid-version							\
	-export-dynamic							\
	-module								\
	$(PLATFORM_LDFLAGS)

tumbler_xdg_cache_la_LIBADD =						\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la	\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(PNG_LIBS)

tumbler_xdg_cache_la_DEPENDENCIES =					\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

# install symlink to 'tumbler-cache-plugin.so'
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(tumbler_plugindir)
	-( cd $(DESTDIR)$(tumbler_plugindir) ; \
		test -x tumbler-xdg-cache.so || \
		ln -sf tumbler-xdg-cache.so tumbler-cache-plugin.so )
endif

EXTRA_DIST = \
	meson.build \
	$(NULL)
