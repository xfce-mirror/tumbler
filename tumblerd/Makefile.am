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
	tumbler-manager-gdbus.c					        \
	tumbler-manager-gdbus.h                                         \
	tumbler-service-gdbus.c					        \
	tumbler-service-gdbus.h                                         \
	tumbler-cache-service-gdbus.h                                   \
	tumbler-cache-service-gdbus.c \
	tumbler-marshal.c \
	tumbler-marshal.h \
	$(NULL)


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
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)	 					\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(PLATFORM_CFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS)                  \
	$(PLATFORM_CPPFLAGS)

tumblerd_LDFLAGS =							\
	-no-undefined							\
	$(PLATFORM_LDFLAGS)

tumblerd_DEPENDENCIES =							\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

tumblerd_LDADD =							\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)                                                \
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)							\
	$(LIBXFCE4UTIL_LIBS)						\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

servicedir = $(datadir)/dbus-1/services
service_in_files = 							\
	org.xfce.Tumbler.Cache1.service.in				\
	org.xfce.Tumbler.Manager1.service.in				\
	org.xfce.Tumbler.Thumbnailer1.service.in

service_DATA = $(service_in_files:org.xfce.Tumbler.%.service.in=@TUMBLER_SERVICE_FILENAME_PREFIX@.%.service)

systemd_userdir = $(prefix)/lib/systemd/user
systemd_user_in_files = tumblerd.service.in

systemd_user_DATA = $(systemd_user_in_files:.service.in=.service)

dbus_xml_in_files = \
	tumbler-cache-service-dbus.xml.in \
	tumbler-manager-dbus.xml.in \
	tumbler-service-dbus.xml.in

dbus_xml_files = $(dbus_xml_in_files:.xml.in=.xml)

@TUMBLER_SERVICE_FILENAME_PREFIX@.%.service: org.xfce.Tumbler.%.service.in
	$(SED) -e "s,\@libdir\@,$(libdir),g" \
	       -e "s,\@TUMBLER_VERSION_API\@,$(TUMBLER_VERSION_API),g" \
	       -e "s,\@TUMBLER_SERVICE_NAME_PREFIX\@,$(TUMBLER_SERVICE_NAME_PREFIX),g" \
	       -e "s,\@TUMBLER_SERVICE_PATH_PREFIX\@,$(TUMBLER_SERVICE_PATH_PREFIX),g" < $< > $@

tumblerd.service: tumblerd.service.in
	$(SED) -e "s,\@libdir\@,$(libdir),g" \
	       -e "s,\@TUMBLER_VERSION_API\@,$(TUMBLER_VERSION_API),g" \
	       -e "s,\@TUMBLER_SERVICE_NAME_PREFIX\@,$(TUMBLER_SERVICE_NAME_PREFIX),g" \
	       -e "s,\@TUMBLER_SERVICE_PATH_PREFIX\@,$(TUMBLER_SERVICE_PATH_PREFIX),g" < $< > $@

%.xml: %.xml.in
	$(SED) -e "s,\@TUMBLER_SERVICE_NAME_PREFIX\@,$(TUMBLER_SERVICE_NAME_PREFIX),g" \
	       -e "s,\@TUMBLER_SERVICE_PATH_PREFIX\@,$(TUMBLER_SERVICE_PATH_PREFIX),g" < $< > $@

tumbler-marshal.h: stamp-tumbler-marshal.h
	@true

stamp-tumbler-marshal.h: tumbler-marshal.list Makefile
	$(AM_V_GEN) ( \
	  cd $(top_builddir)/tumblerd \
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
	  cd $(top_builddir)/tumblerd \
	  && echo "#include \"tumbler-marshal.h\"" > xgen-emc \
	  && ( glib-genmarshal \
	  	--prefix=tumbler_marshal \
	  	--body $(srcdir)/tumbler-marshal.list ) >> xgen-emc \
	  && cp xgen-emc tumbler-marshal.c \
	  && rm -f xgen-emc \
	)

confdir = $(sysconfdir)/xdg/tumbler
conf_DATA = \
	tumbler.rc

CLEANFILES =								\
	$(systemd_user_DATA)						\
	$(service_DATA)

EXTRA_DIST =								\
	$(systemd_user_in_files)					\
	$(service_in_files)						\
	tumbler.rc							\
	$(dbus_xml_in_files) \
	tumbler-marshal.list \
	$(NULL)

DISTCLEANFILES =							\
	$(tumblerd_built_sources) \
	$(dbus_xml_files) \
	stamp-tumbler-marshal.h \
	$(NULL)

BUILT_SOURCES =								\
	$(tumblerd_built_sources)

tumbler-manager-gdbus.h:
tumbler-manager-gdbus.c: tumbler-manager-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix @TUMBLER_SERVICE_NAME_PREFIX@.Manager1 \
	--c-namespace Tumbler 	\
	--generate-c-code tumbler-manager-gdbus \
	tumbler-manager-dbus.xml

tumbler-service-gdbus.h:
tumbler-service-gdbus.c: tumbler-service-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix @TUMBLER_SERVICE_NAME_PREFIX@.Thumbnailer1 \
	--c-namespace Tumbler 	\
	--generate-c-code tumbler-service-gdbus \
	tumbler-service-dbus.xml

tumbler-cache-service-gdbus.h:
tumbler-cache-service-gdbus.c: tumbler-cache-service-dbus.xml Makefile
	$(AM_V_GEN) $(GDBUS_CODEGEN) \
	--interface-prefix @TUMBLER_SERVICE_NAME_PREFIX@.Cache1 \
	--c-namespace Tumbler 	\
	--generate-c-code tumbler-cache-service-gdbus \
	tumbler-cache-service-dbus.xml
