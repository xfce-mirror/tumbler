project(
  'tumbler',
  'c',
version : '4.20.0-dev',
  license : 'GPL-2.0-or-later',
  meson_version : '>= 0.61.0',
  default_options : ['c_std=gnu11', 'buildtype=debugoptimized', 'warning_level=2']
)

project_namespace = 'xfce'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
tumbler_version_api = '1'
tumbler_plugin_directory = get_option('prefix') / get_option('libdir') / 'tumbler-@0@'.format(tumbler_version_api) / 'plugins'

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'gdk-pixbuf': '>= 2.42.8',
  'glib': '>= 2.72.0',
  'libxfce4util': '>= 4.17.1',

  'curl': '>= 7.32.0',
  'ffmpeg': '>= 2.2.2',
  'freetype': '>= 2.0.0',
  'libgepub': '>= 0.6.0',
  'gstreamer': '>= 1.0.0',
  'libjpeg': '>= 2.0.0',
  'libgsf': '>= 1.14.9',
  'poppler-glib': '>= 0.12.0',
  'libopenraw': '>= 0.1.0',
  'libpng': '>= 1.6.0',
}

gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: dependency_versions['gdk-pixbuf'])
glib = dependency('glib-2.0', version: dependency_versions['glib'])
gio = dependency('gio-2.0', version: dependency_versions['glib'])
gio_unix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
gmodule = dependency('gmodule-2.0', version: dependency_versions['glib'])
gthread = dependency('gthread-2.0', version: dependency_versions['glib'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['libxfce4util'])
libm = cc.find_library('m', required: true)

xdt_gen_visibility = find_program('xdt-gen-visibility', required: true)

enable_thumbnailer = {}
thumbnailer_deps = {}
base_deps = [gdk_pixbuf, glib, gio]

deps = base_deps
deps += dependency('libcurl', version: dependency_versions['curl'], required: get_option('cover-thumbnailer'))
thumbnailer_deps += {'cover': deps}
enable_thumbnailer += {'cover': deps[-1].found()}

deps = base_deps
thumbnailer_deps += {'desktop': deps}
enable_thumbnailer += {'desktop': not get_option('desktop-thumbnailer').disabled()}

deps = base_deps
deps += dependency('libffmpegthumbnailer', version: dependency_versions['ffmpeg'], required: get_option('ffmpeg-thumbnailer'))
thumbnailer_deps += {'ffmpeg': deps}
enable_thumbnailer += {'ffmpeg': deps[-1].found()}

deps = base_deps
deps += dependency('freetype2', version: dependency_versions['freetype'], required: get_option('font-thumbnailer'))
thumbnailer_deps += {'font': deps}
enable_thumbnailer += {'font': deps[-1].found()}

gepub = dependency('libgepub-0.7', version: dependency_versions['libgepub'], required: false)
if not gepub.found()
  gepub = dependency('libgepub-0.6', version: dependency_versions['libgepub'], required: get_option('gepub-thumbnailer'))
endif
deps = base_deps + gepub
thumbnailer_deps += {'gepub': deps}
enable_thumbnailer += {'gepub': deps[-1].found()}

deps = base_deps
deps += dependency('gstreamer-1.0', version: dependency_versions['gstreamer'], required: get_option('gst-thumbnailer'))
if deps[-1].found()
  deps += dependency('gstreamer-tag-1.0', version: dependency_versions['gstreamer'], required: get_option('gst-thumbnailer'))
endif
thumbnailer_deps += {'gst': deps}
enable_thumbnailer += {'gst': deps[-1].found()}

deps = base_deps
deps += dependency('libjpeg', version: dependency_versions['libjpeg'], required: get_option('jpeg-thumbnailer'))
thumbnailer_deps += {'jpeg': deps}
enable_thumbnailer += {'jpeg': deps[-1].found()}

deps = base_deps
deps += dependency('libgsf-1', version: dependency_versions['libgsf'], required: get_option('odf-thumbnailer'))
thumbnailer_deps += {'odf': deps}
enable_thumbnailer += {'odf': deps[-1].found()}

deps = base_deps
thumbnailer_deps += {'pixbuf': deps}
enable_thumbnailer += {'pixbuf': not get_option('pixbuf-thumbnailer').disabled()}

deps = base_deps
deps += dependency('poppler-glib', version: dependency_versions['poppler-glib'], required: get_option('poppler-thumbnailer'))
thumbnailer_deps += {'poppler': deps}
enable_thumbnailer += {'poppler': deps[-1].found()}

deps = base_deps
deps += dependency('libopenraw-gnome-0.3', version: dependency_versions['libopenraw'], required: false)
if not deps[-1].found()
  deps = base_deps
  deps += dependency('libopenraw-gnome-0.2', version: dependency_versions['libopenraw'], required: false)
  if not deps[-1].found()
    deps = base_deps
    deps += dependency('libopenraw-gnome-0.1', version: dependency_versions['libopenraw'], required: get_option('raw-thumbnailer'))
  endif
endif
thumbnailer_deps += {'raw': deps}
enable_thumbnailer += {'raw': deps[-1].found()}

xdg_cache_deps = [gdk_pixbuf, glib, gio, libxfce4util, libm]
xdg_cache_deps += dependency('libpng', version: dependency_versions['libpng'], required: get_option('xdg-cache'))
enable_xdg_cache = xdg_cache_deps[-1].found()

feature_cflags = []

gnu_symbol_visibility = 'default'
if get_option('visibility')
  gnu_symbol_visibility = 'hidden'
  feature_cflags += '-DENABLE_SYMBOL_VISIBILITY=1'
endif

tumbler_service_name_prefix = get_option('service-name-prefix')
if tumbler_service_name_prefix == ''
  tumbler_service_name_prefix = 'org.freedesktop.thumbnails'
  tumbler_service_filename_prefix = 'org.xfce.Tumbler'
else
  tumbler_service_filename_prefix = tumbler_service_name_prefix
endif
tumbler_service_path_prefix = '/@0@'.format('/'.join(tumbler_service_name_prefix.split('.')))

need_libintl = false
has_bind_textdomain_codeset = cc.has_function('bind_textdomain_codeset')
if not has_bind_textdomain_codeset
  libintl = cc.find_library('intl', required: false)
  if libintl.found()
    has_bind_textdomain_codeset = cc.has_function(
      'bind_textdomain_codeset',
      dependencies: [
        libintl,
      ],
    )
    if has_bind_textdomain_codeset
      need_libintl = true
    endif
  endif
endif
if has_bind_textdomain_codeset
  feature_cflags += '-DHAVE_BIND_TEXTDOMAIN_CODESET=1'
endif

functions = [
  'mmap',
  'sched_getparam',
  'sched_setscheduler',
]
foreach function : functions
  if cc.has_function(function)
    feature_cflags += '-DHAVE_@0@=1'.format(function.to_upper())
  endif
endforeach

headers = [
  'fcntl.h',
  'linux/sched.h',
  'math.h',
  'memory.h',
  'pwd.h',
  'sched.h',
  'setjmp.h',
  'stdio.h',
  'stdlib.h',
  'string.h',
  'syscall.h',
  'unistd.h',
  'sys/mman.h',
  'sys/select.h',
  'sys/stat.h',
  'sys/types.h',
]
foreach header : headers
  if cc.check_header(header)
    feature_cflags += '-DHAVE_@0@=1'.format(header.underscorify().to_upper())
  endif
endforeach

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

subdir('tumbler')
subdir('docs' / 'reference' / 'tumbler')
subdir('icons')
subdir('plugins')
subdir('plugins' / 'xdg-cache')
subdir('po')
subdir('tumblerd')
