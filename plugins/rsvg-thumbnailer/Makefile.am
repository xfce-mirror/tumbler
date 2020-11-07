if TUMBLER_RSVG_THUMBNAILER

tumbler_plugindir = $(libdir)/tumbler-$(TUMBLER_VERSION_API)/plugins
tumbler_plugin_LTLIBRARIES =						\
	tumbler-rsvg-thumbnailer.la

tumbler_rsvg_thumbnailer_la_SOURCES =           \
	rsvg-thumbnailer-plugin.c					\
	rsvg-thumbnailer-provider.c					\
	rsvg-thumbnailer-provider.h					\
	rsvg-thumbnailer.c                          \
	rsvg-thumbnailer.h

tumbler_rsvg_thumbnailer_la_CFLAGS =					\
	-I$(top_builddir)						\
	-I$(top_builddir)/plugins					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/plugins						\
	-DG_LOG_DOMAIN=\"tumbler-rsvg-thumbnailer\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GDK_PIXBUF_CFLAGS)					\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(CAIRO_CFLAGS)                         \
	$(RSVG_CFLAGS)                          \
	$(GDK3_CFLAGS)                          \
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

tumbler_rsvg_thumbnailer_la_LDFLAGS =                                  \
	-avoid-version							\
	-export-dynamic							\
	-module								\
	$(PLATFORM_LDFLAGS)

tumbler_rsvg_thumbnailer_la_LIBADD =					\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la	\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(CAIRO_LIBS)                           \
	$(RSVG_LIBS)                            \
	$(GDK3_LIBS)                            \
	-lm

tumbler_rsvg_thumbnailer_la_DEPENDENCIES =				\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

endif
