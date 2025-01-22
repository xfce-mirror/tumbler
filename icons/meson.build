sizes = [32, 64]

foreach size : sizes
  install_data(
    '@0@x@0@'.format(size) / 'org.xfce.tumbler.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach
