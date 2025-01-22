if enable_xdg_cache
  xdg_cache_name = 'tumbler-xdg-cache'
  xdg_cache_install_dir = tumbler_plugin_directory / 'cache'

  shared_module(
    xdg_cache_name,
    [
      'xdg-cache-cache.c',
      'xdg-cache-cache.h',
      'xdg-cache-plugin.c',
      'xdg-cache-thumbnail.c',
      'xdg-cache-thumbnail.h',
    ],
    name_prefix: '',
    gnu_symbol_visibility: 'hidden',
    c_args: [
      '-DG_LOG_DOMAIN="@0@"'.format('tumbler-xdg-cache'),
    ],
    include_directories: [include_directories('..' / '..')],
    dependencies: xdg_cache_deps,
    link_with: [tumbler],
    install: true,
    install_dir: xdg_cache_install_dir,
  )

  install_symlink(
    'tumbler-cache-plugin.so',
    install_dir: xdg_cache_install_dir,
    pointing_to: '@0@.so'.format(xdg_cache_name),
  )
endif
