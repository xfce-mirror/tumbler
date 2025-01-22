# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
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
# 

AUTOMAKE_OPTIONS = 1.8

# Name of the module
DOC_MODULE=tumbler

# Top-level SGML file
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan
# e.g. SCAN_OPTIONS=--rebuild-sections --rebuild-types to update files under
# version control, or SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED"
SCAN_OPTIONS=

# Directory containing the source code
DOC_SOURCE_DIR=$(top_srcdir)/tumbler

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=

# Used for dependencies
HFILE_GLOB=$(top_srcdir)/tumbler/*.h
CFILE_GLOB=$(top_srcdir)/tumbler/*.c

 # Header files to ignore when scanning
IGNORE_HFILES = \
	tumbler-marshal.h \
	$(NULL)
 
# Extra files to add when scanning (relative to $srcdir)
EXTRA_HFILES=

# Images to copy into HTML directory
HTML_IMAGES =

# Extra SGML files that are included by DOC_MAIN_SGML_FILE
content_files =								\
	version.xml

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
AM_CPPFLAGS =	 							\
	-I$(top_srcdir) 						\
	-I$(top_builddir) 						\
	$(DBUS_CFLAGS)							\
	$(DBUS_GLIB_CFLAGS)						\
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(GTK_DOC_EXTRA_CFLAGS)						\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)						\
	$(PNG_CFLAGS)

GTKDOC_LIBS =								\
	$(DBUS_LIBS)							\
	$(DBUS_GLIB_LIBS)						\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)							\
	$(GLIB_LIBS)							\
	$(PNG_LIBS)							\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST +=								\
	version.xml.in

# Required for make distcheck
dist-hook: all
