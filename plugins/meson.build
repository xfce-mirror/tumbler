thumbnailer_names = [
  'cover',
  'desktop',
  'ffmpeg',
  'font',
  'gepub',
  'gst',
  'jpeg',
  'odf',
  'pixbuf',
  'poppler',
  'raw',
]

foreach name : thumbnailer_names
  if enable_thumbnailer[name]
    thumbnailer_sources = [
      '@0@-thumbnailer'.format(name) / '@0@-thumbnailer-plugin.c'.format(name),
      '@0@-thumbnailer'.format(name) / '@0@-thumbnailer-provider.c'.format(name),
      '@0@-thumbnailer'.format(name) / '@0@-thumbnailer-provider.h'.format(name),
      '@0@-thumbnailer'.format(name) / '@0@-thumbnailer.c'.format(name),
      '@0@-thumbnailer'.format(name) / '@0@-thumbnailer.h'.format(name),
    ]

    # https://gitlab.gnome.org/GNOME/libgepub/-/merge_requests/17
    if name == 'gepub' and gepub.version().version_compare('<=0.7.1')
      thumbnailer_sources += custom_target(
        'config.h',
        output: 'config.h',
        command: ['sh', '-c', '> @OUTPUT@'],
      )
    endif

    shared_module(
      'tumbler-@0@-thumbnailer'.format(name),
      thumbnailer_sources,
      name_prefix: '',
      gnu_symbol_visibility: 'hidden',
      c_args: [
        '-DG_LOG_DOMAIN="tumbler-@0@-thumbnailer"'.format(name),
      ],
      include_directories: [include_directories('..')],
      dependencies: thumbnailer_deps[name],
      link_with: [tumbler],
      install: true,
      install_dir: tumbler_plugin_directory,
    )
  endif
endforeach
