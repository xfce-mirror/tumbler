if TUMBLER_GEPUB_THUMBNAILER

tumbler_plugindir = $(libdir)/tumbler-$(TUMBLER_VERSION_API)/plugins
tumbler_plugin_LTLIBRARIES =						\
	tumbler-gepub-thumbnailer.la

tumbler_gepub_thumbnailer_la_SOURCES =                                  \
	gepub-thumbnailer-plugin.c					\
	gepub-thumbnailer-provider.c					\
	gepub-thumbnailer-provider.h					\
	gepub-thumbnailer.c						\
	gepub-thumbnailer.h

tumbler_gepub_thumbnailer_la_CFLAGS =					\
	-I$(top_builddir)						\
	-I$(top_builddir)/plugins					\
	-I$(top_srcdir)							\
	-I$(top_srcdir)/plugins						\
	-DG_LOG_DOMAIN=\"tumbler-gepub-thumbnailer\"			\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	$(GDK_PIXBUF_CFLAGS)						\
	$(GIO_CFLAGS)							\
	$(GLIB_CFLAGS)							\
	$(GEPUB_CFLAGS)						\
	$(PLATFORM_CFLAGS)						\
	$(PLATFORM_CPPFLAGS)

tumbler_gepub_thumbnailer_la_LDFLAGS =                                  \
	-avoid-version							\
	-export-dynamic							\
	-module								\
	$(PLATFORM_LDFLAGS)

tumbler_gepub_thumbnailer_la_LIBADD =					\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la	\
	$(GDK_PIXBUF_LIBS)						\
	$(GIO_LIBS)							\
	$(GLIB_LIBS)							\
	$(GEPUB_LIBS)                                                   \
	-lm

tumbler_gepub_thumbnailer_la_DEPENDENCIES =				\
	$(top_builddir)/tumbler/libtumbler-$(TUMBLER_VERSION_API).la

endif
