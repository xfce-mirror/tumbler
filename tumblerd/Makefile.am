# vi:set ts=8 sw=8 noet ai nocindent:
#-
# Copyright (c) 2009-2011 Jannis Pohlmann <jannis@xfce.org>
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

tumblerddir =								\
	$(libdir)/tumbler-$(TUMBLER_VERSION_API)

tumblerd_PROGRAMS =							\
	tumblerd

tumblerd_built_sources =						\
	tumbler-cache-service-dbus-bindings.h				\
	tumbler-manager-dbus-bindings.h					\
	tumbler-service-dbus-bindings.h

tumblerd_SOURCES =							\
	main.c								\
	tumbler-cache-service.c						\
	tumbler-cache-service.h						\
	tumbler-component.c						\
	tumbler-component.h						\
	tumbler-lifecycle-manager.c					\
	tumbler-lifecycle-manager.h					\
	tumbler-manager.c						\
	tumbler-manager.h						\
	tumbler-registry.c						\
	tumbler-registry.h						\
	tumbler-scheduler.c						\
	tumbler-scheduler.h						\
	tumbler-service.c						\
	tumbler-service.h						\
	tumbler-specialized-thumbnailer.c				\
	tumbler-specialized-thumbnailer.h				\
	tumbler-lifo-scheduler.c					\
	tumbler-lifo-scheduler.h					\
	tumbler-group-scheduler.c					\
	tumbler-group-scheduler.h					\
	tumbler-utils.h							\
	$(tumblerd_built_sources)

tumblerd_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DG_LOG_DOMAIN=\"tumblerd\"					\
	-I$(top_srcdir)							\
	$(DBUS_CFLAGS)							\
	$(DBUS_GLIB_CFLAGS)						\
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

tumblerd_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

tumblerd_DEPENDENCIES =							\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

tumblerd_LDADD =							\
	$(DBUS_LIBS)							\
	$(DBUS_GLIB_LIBS)						\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)							\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

servicedir = $(datadir)/dbus-1/services
service_in_files = 							\
	org.xfce.Tumbler.Cache1.service.in				\
	org.xfce.Tumbler.Manager1.service.in				\
	org.xfce.Tumbler.Thumbnailer1.service.in

service_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s,\@libdir\@,$(libdir),g"				\
	    -e "s,\@TUMBLER_VERSION_API\@,$(TUMBLER_VERSION_API),g" < $< > $@

CLEANFILES =								\
	$(service_DATA)

EXTRA_DIST =								\
	$(service_in_files)						\
	tumbler-cache-service-dbus.xml					\
	tumbler-manager-dbus.xml					\
	tumbler-service-dbus.xml

DISTCLEANFILES =							\
	$(tumblerd_built_sources)

BUILT_SOURCES =								\
	$(tumblerd_built_sources)

tumbler-cache-service-dbus-bindings.h: tumbler-cache-service-dbus.xml Makefile
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --mode=glib-server --prefix=tumbler_cache_service $< > $@

tumbler-manager-dbus-bindings.h: tumbler-manager-dbus.xml Makefile
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --mode=glib-server --prefix=tumbler_manager $< > $@

tumbler-service-dbus-bindings.h: tumbler-service-dbus.xml Makefile
	$(AM_V_GEN) $(DBUS_BINDING_TOOL) --mode=glib-server --prefix=tumbler_service $< > $@
