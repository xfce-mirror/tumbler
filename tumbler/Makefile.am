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

tumbler_SOURCES =							\
	main.c

tumbler_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DG_LOG_DOMAIN=\"tumbler\"					\
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
	$(service_in_files)
