if TUMBLER_WEBP_THUMBNAILER

tumbler_plugindir = $(libdir)/tumbler-$(TUMBLER_VERSION_API)/plugins
tumbler_plugin_LTLIBRARIES =						\
	tumbler-webp-thumbnailer.la

tumbler_webp_thumbnailer_la_SOURCES =           \
	webp-thumbnailer-plugin.c					\
	webp-thumbnailer-provider.c					\
	webp-thumbnailer-provider.h					\
	webp-thumbnailer.c                          \
	webp-thumbnailer.h

tumbler_webp_thumbnailer_la_CFLAGS =					\
	-I$(top_builddir)						\
	-I$(top_builddir)/plugins					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/plugins						\
	-DG_LOG_DOMAIN=\"tumbler-webp-thumbnailer\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GDK_PIXBUF_CFLAGS)					\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(CAIRO_CFLAGS)                         \
	$(WEBP_CFLAGS)                          \
	$(GDK3_CFLAGS)                          \
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

tumbler_webp_thumbnailer_la_LDFLAGS =                                  \
	-avoid-version							\
	-export-dynamic							\
	-module								\
	$(PLATFORM_LDFLAGS)

tumbler_webp_thumbnailer_la_LIBADD =					\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la	\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(CAIRO_LIBS)                           \
	$(WEBP_LIBS)                            \
	$(GDK3_LIBS)                            \
	-lm

tumbler_webp_thumbnailer_la_DEPENDENCIES =				\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

endif
