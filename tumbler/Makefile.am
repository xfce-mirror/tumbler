# vi:set ts=8 sw=8 noet ai nocindent:
#-
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

lib_LTLIBRARIES =							\
	libtumbler-1.la

libtumbler_built_public_sources =					\
	tumbler-marshal.h

libtumbler_built_sources =						\
	$(libtumbler_built_public_sources)				\
	tumbler-marshal.c

libtumbler_headers =							\
	tumbler-config.h						\
	tumbler-enum-types.h						\
	tumbler-error.h							\
	tumbler-marshal.h						\
	tumbler-thumbnail.h

libtumbler_sources =							\
	tumbler-config.c						\
	tumbler-enum-types.c						\
	tumbler-marshal.c						\
	tumbler-thumbnail.c

libtumblerincludedir = $(includedir)/tumbler-$(TUMBLER_VERSION_API)/tumbler

libtumblerinclude_HEADERS =						\
	$(libtumbler_headers)

libtumbler_1_la_SOURCES =						\
	$(libtumbler_sources)						\
	$(libtumbler_headers)

libtumbler_1_la_CFLAGS =						\
	-DDATADIR=\"$(datadir)\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DG_LOG_DOMAIN=\"tumbler\"					\
	-DTUMBLER_COMPILATION						\
	-DTUMBLER_VERSION_API=\"$(TUMBLER_VERSION_API)\"		\
	-I$(top_srcdir)							\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(PLATFORM_CFLAGS)

libtumbler_1_la_LDFLAGS =						\
	-no-undefined							\
	-export-dynamic							\
	-version-info $(TUMBLER_VERINFO)				\
	$(PLATFORM_LDFLAGS)

libtumbler_1_la_LIBADD =						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)

EXTRA_DIST =								\
	tumbler-config.h.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tumbler-1.pc

if MAINTAINER_MODE

DISTCLEANFILES =							\
	stamp-tumbler-marshal.h						\
	tumbler-1.pc							\
	tumbler-config.h						\
	$(libtumbler_built_sources)

BUILT_SOURCES =								\
	$(tumbler_built_sources)

tumbler-marshal.h: stamp-tumbler-marshal.h
	@true

stamp-tumbler-marshal.h: tumbler-marshal.list Makefile
	( cd $(srcdir) && glib-genmarshal \
		--prefix=tumbler_marshal \
		--header tumbler-marshal.list ) >> xgen-emh \
	&& ( cmp -s xgen-emh tumbler-marshal.h || cp xgen-emh tumbler-marshal.h ) \
	&& rm -f xgen-emh \
	&& echo timestamp > $(@F)

tumbler-marshal.c: tumbler-marshal.list Makefile
	( cd $(srcdir) && glib-genmarshal \
		--prefix=tumbler_marshal \
		--body tumbler-marshal.list ) >> xgen-emc \
	&& cp xgen-emc tumbler-marshal.c \
	&& rm -f xgen-emc

endif
