tumbler_libname = 'tumbler'
tumbler_pkgname = '@0@-@1@'.format(tumbler_libname, tumbler_version_api)

tumbler_install_subdir = tumbler_pkgname / tumbler_libname
tumbler_install_dir = get_option('prefix') / get_option('includedir') / tumbler_install_subdir

tumbler_headers = [
  'tumbler-abstract-thumbnailer.h',
  'tumbler-cache-plugin.h',
  'tumbler-cache.h',
  'tumbler-enum-types.h',
  'tumbler-error.h',
  'tumbler-file-info.h',
  'tumbler-provider-factory.h',
  'tumbler-provider-plugin.h',
  'tumbler-thumbnail-flavor.h',
  'tumbler-thumbnail.h',
  'tumbler-thumbnailer-provider.h',
  'tumbler-thumbnailer.h',
  'tumbler-util.h',
  'tumbler.h',
]

tumbler_headers += configure_file(
  configuration: configuration_data({
    'TUMBLER_VERSION_MAJOR': version_parts[0],
    'TUMBLER_VERSION_MINOR': version_parts[1],
    'TUMBLER_VERSION_MICRO': version_parts[2],
  }),
  input: 'tumbler-config.h.in',
  output: 'tumbler-config.h',
  install: false,
)

tumbler_sources = [
  'tumbler-abstract-thumbnailer.c',
  'tumbler-cache-plugin.c',
  'tumbler-cache.c',
  'tumbler-config.c',
  'tumbler-enum-types.c',
  'tumbler-file-info.c',
  'tumbler-provider-factory.c',
  'tumbler-provider-plugin.c',
  'tumbler-thumbnail-flavor.c',
  'tumbler-thumbnail.c',
  'tumbler-thumbnailer-provider.c',
  'tumbler-thumbnailer.c',
  'tumbler-util.c',
]

tumbler_sources += gnome.genmarshal(
  'tumbler-marshal',
  sources: 'tumbler-marshal.list',
  prefix: 'tumbler_marshal',
  internal: true,
  install_header: false,
)

tumbler_sources += custom_target(
  'tumbler-visibility.h',
  input: 'tumbler.symbols',
  output: 'tumbler-visibility.h',
  command: [xdt_gen_visibility, '--kind=header', '@INPUT@', '@OUTPUT@'],
)
tumbler_sources += custom_target(
  'tumbler-visibility.c',
  input: 'tumbler.symbols',
  output: 'tumbler-visibility.c',
  command: [xdt_gen_visibility, '--kind=source', '@INPUT@', '@OUTPUT@'],
)

tumbler_deps = [
  gdk_pixbuf,
  glib,
  gio,
  gmodule,
  gthread,
  libxfce4util,
  libm,
]
if need_libintl
  tumbler_deps += libintl
endif

tumbler_so_version = '0'
tumbler_version = '@0@.0.0'.format(tumbler_so_version)

tumbler = library(
  tumbler_pkgname,
  sources: tumbler_sources,
  version: tumbler_version,
  soversion: tumbler_so_version,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format(tumbler_libname),
	  '-DTUMBLER_PLUGIN_DIRECTORY="@0@"'.format(tumbler_plugin_directory),
    '-DTUMBLER_COMPILATION=1',
  ],
  include_directories: [
    include_directories('..'),
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  dependencies: tumbler_deps,
  install: true,
)

install_headers(
  tumbler_headers,
  subdir: tumbler_install_subdir,
)

pkgconfig.generate(
  libraries: [
    tumbler,
  ],
  version: meson.project_version(),
  name: tumbler_libname,
  filebase: tumbler_pkgname,
  description: 'Supporting library for tumbler',
  subdirs: [
    tumbler_pkgname,
  ],
  variables: [
    'tumbler_api_version=@0@'.format(tumbler_version_api),
  ],
  install_dir: get_option('prefix') / get_option('libdir') / 'pkgconfig',
)

tumbler_dep = declare_dependency(
  dependencies: tumbler_deps,
  link_with: tumbler,
)
