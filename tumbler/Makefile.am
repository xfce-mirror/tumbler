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

libtumbler_built_sources =						\
	tumbler-marshal.c \
	tumbler-marshal.h \
	tumbler-visibility.c \
	tumbler-visibility.h \
	$(NULL)

libtumbler_headers =							\
	tumbler-abstract-thumbnailer.h					\
	tumbler-cache.h							\
	tumbler-cache-plugin.h						\
	tumbler-config.h						\
	tumbler-enum-types.h						\
	tumbler-error.h							\
	tumbler-file-info.h						\
	tumbler-provider-factory.h					\
	tumbler-provider-plugin.h					\
	tumbler-thumbnailer-provider.h					\
	tumbler-thumbnailer.h						\
	tumbler-thumbnail.h						\
	tumbler-thumbnail-flavor.h					\
	tumbler-util.h							\
	tumbler.h

libtumbler_sources =							\
	$(libtumbler_built_sources) \
	tumbler-abstract-thumbnailer.c					\
	tumbler-cache.c							\
	tumbler-cache-plugin.c						\
	tumbler-config.c						\
	tumbler-enum-types.c						\
	tumbler-file-info.c						\
	tumbler-provider-factory.c					\
	tumbler-provider-plugin.c					\
	tumbler-thumbnailer-provider.c					\
	tumbler-thumbnailer.c						\
	tumbler-thumbnail.c						\
	tumbler-thumbnail-flavor.c					\
	tumbler-util.c

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
	-DTUMBLER_PLUGIN_DIRECTORY=\"$(libdir)/tumbler-$(TUMBLER_VERSION_API)/plugins\" \
	-I$(top_srcdir)							\
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(GMODULE_CFLAGS)						\
	$(GTHREAD_CFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS) \
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

libtumbler_1_la_LDFLAGS =						\
	-no-undefined							\
	-export-dynamic							\
	-version-info $(TUMBLER_VERINFO)				\
	$(PLATFORM_LDFLAGS)

libtumbler_1_la_LIBADD =						\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(GMODULE_LIBS)							\
	$(GTHREAD_LIBS) \
	$(LIBXFCE4UTIL_LIBS)

EXTRA_DIST =								\
	tumbler-config.h.in						\
	tumbler-marshal.list \
	tumbler.symbols \
	meson.build \
	$(NULL)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tumbler-1.pc

DISTCLEANFILES =							\
	stamp-tumbler-marshal.h						\
	tumbler-config.h						\
	$(libtumbler_built_sources)

BUILT_SOURCES =								\
	$(libtumbler_built_sources)

tumbler-marshal.h: stamp-tumbler-marshal.h
	@true

stamp-tumbler-marshal.h: tumbler-marshal.list Makefile
	$(AM_V_GEN) ( \
	  cd $(top_builddir)/tumbler \
	  && echo "#ifndef __TUMBLER_MARSHAL_H__" > xgen-emh \
	  && echo "#define __TUMBLER_MARSHAL_H__" >> xgen-emh \
	  && ( glib-genmarshal \
	  	--prefix=tumbler_marshal \
	  	--header $(srcdir)/tumbler-marshal.list ) >> xgen-emh \
	  && echo "#endif /* !__TUMBLER_MARSHAL_H__ */" >> xgen-emh \
	  && ( cmp -s xgen-emh tumbler-marshal.h \
	  	|| cp xgen-emh tumbler-marshal.h ) \
	  && rm -f xgen-emh \
	  && echo timestamp > $(@F) \
	)

tumbler-marshal.c: tumbler-marshal.list Makefile
	$(AM_V_GEN) ( \
	  cd $(top_builddir)/tumbler \
	  && echo "#include \"tumbler-marshal.h\"" > xgen-emc \
	  && ( glib-genmarshal \
	  	--prefix=tumbler_marshal \
	  	--body $(srcdir)/tumbler-marshal.list ) >> xgen-emc \
	  && cp xgen-emc tumbler-marshal.c \
	  && rm -f xgen-emc \
	)

tumbler-visibility.h: tumbler.symbols Makefile
	$(AM_V_GEN) $(XDT_GEN_VISIBILITY) --kind=header $< $@

tumbler-visibility.c: tumbler.symbols Makefile
	$(AM_V_GEN) $(XDT_GEN_VISIBILITY) --kind=source $< $@

# Required for gtk-doc and make distcheck
dist-hook: all
