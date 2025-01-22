tumblerd_sources = [
  'main.c',
  'tumbler-cache-service.c',
  'tumbler-cache-service.h',
  'tumbler-component.c',
  'tumbler-component.h',
  'tumbler-group-scheduler.c',
  'tumbler-group-scheduler.h',
  'tumbler-lifecycle-manager.c',
  'tumbler-lifecycle-manager.h',
  'tumbler-lifo-scheduler.c',
  'tumbler-lifo-scheduler.h',
  'tumbler-manager.c',
  'tumbler-manager.h',
  'tumbler-registry.c',
  'tumbler-registry.h',
  'tumbler-scheduler.c',
  'tumbler-scheduler.h',
  'tumbler-service.c',
  'tumbler-service.h',
  'tumbler-specialized-thumbnailer.c',
  'tumbler-specialized-thumbnailer.h',
  'tumbler-utils.h',
]

tumblerd_sources += gnome.genmarshal(
  'tumbler-marshal',
  sources: 'tumbler-marshal.list',
  prefix: 'tumbler_marshal',
  internal: true,
  install_header: false,
)

configure_file(
  configuration: configuration_data({
    'libdir': get_option('prefix') / get_option('libdir'),
    'TUMBLER_VERSION_API': tumbler_version_api,
    'TUMBLER_SERVICE_NAME_PREFIX': tumbler_service_name_prefix,
    'TUMBLER_SERVICE_PATH_PREFIX': tumbler_service_path_prefix,
  }),
  input: 'tumblerd.service.in',
  output: 'tumblerd.service',
  install: true,
  install_dir: get_option('prefix') / 'lib' / 'systemd' / 'user'
)

services = {
  'Cache1': 'tumbler-cache-service',
  'Manager1': 'tumbler-manager',
  'Thumbnailer1': 'tumbler-service',
}
foreach service_name, basename : services
  configure_file(
    configuration: configuration_data({
      'libdir': get_option('prefix') / get_option('libdir'),
      'TUMBLER_VERSION_API': tumbler_version_api,
      'TUMBLER_SERVICE_NAME_PREFIX': tumbler_service_name_prefix,
      'TUMBLER_SERVICE_PATH_PREFIX': tumbler_service_path_prefix,
    }),
    input: 'org.xfce.Tumbler.@0@.service.in'.format(service_name),
    output: '@0@.@1@.service'.format(tumbler_service_filename_prefix, service_name),
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'dbus-1' / 'services'
  )

  file = configure_file(
    configuration: configuration_data({
      'TUMBLER_SERVICE_NAME_PREFIX': tumbler_service_name_prefix,
      'TUMBLER_SERVICE_PATH_PREFIX': tumbler_service_path_prefix,
    }),
    input: '@0@-dbus.xml.in'.format(basename),
    output: '@0@-dbus.xml'.format(basename),
    install: false,
  )

  tumblerd_sources += gnome.gdbus_codegen(
    '@0@-gdbus'.format(basename),
    sources: file,
    interface_prefix: '@0@.@1@'.format(tumbler_service_name_prefix, service_name),
    namespace: 'Tumbler',
    install_header: false,
  )
endforeach

install_data(
  'tumbler.rc',
  install_dir: get_option('prefix') / get_option('sysconfdir') / 'xdg' / 'tumbler',
)

executable(
  'tumblerd',
  tumblerd_sources,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('tumblerd'),
    '-DTUMBLER_SERVICE_NAME_PREFIX="@0@"'.format(tumbler_service_name_prefix),
    '-DTUMBLER_SERVICE_PATH_PREFIX="@0@"'.format(tumbler_service_path_prefix),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gdk_pixbuf,
    glib,
    gio,
    gio_unix,
    gthread,
    libxfce4util,
  ],
  link_with: [
    tumbler,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / 'tumbler-@0@'.format(tumbler_version_api),
)
