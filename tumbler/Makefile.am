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

libexec_PROGRAMS =							\
	tumbler

tumbler_built_sources =							\
	tumbler-manager-dbus-bindings.h					\
	tumbler-marshal.c						\
	tumbler-marshal.h

tumbler_SOURCES =							\
	main.c								\
	tumbler-builtin-thumbnailers.c					\
	tumbler-builtin-thumbnailers.h					\
	tumbler-builtin-thumbnailer.c					\
	tumbler-builtin-thumbnailer.h					\
	tumbler-manager.c						\
	tumbler-manager.h						\
	tumbler-registry.c						\
	tumbler-registry.h						\
	tumbler-service.c						\
	tumbler-service.h						\
	tumbler-specialized-thumbnailer.c				\
	tumbler-specialized-thumbnailer.h				\
	tumbler-thumbnailer.c						\
	tumbler-thumbnailer.h						\
	tumbler-utils.h							\
	$(tumbler_built_sources)

tumbler_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DG_LOG_DOMAIN=\"tumbler\"					\
	-I$(top_srcdir)							\
	$(DBUS_CFLAGS)							\
	$(DBUS_GLIB_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)

tumbler_LDFLAGS =							\
	-no-undefined

tumbler_LDADD =								\
	$(DBUS_LIBS)							\
	$(DBUS_GLIB_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.xfce.Tumbler.service.in
service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libexecdir\@,$(libexecdir),g" < $< > $@

CLEANFILES =								\
	$(service_DATA)

EXTRA_DIST =								\
	$(service_in_files)						\
	tumbler-manager-dbus.xml	

if MAINTAINER_MODE

DISTCLEANFILES =							\
	stamp-tumbler-marshal.h						\
	$(tumbler_built_sources)

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

tumbler-manager-dbus-bindings.h: tumbler-manager-dbus.xml Makefile
	dbus-binding-tool --mode=glib-server --prefix=tumbler_manager $< > $@

endif
