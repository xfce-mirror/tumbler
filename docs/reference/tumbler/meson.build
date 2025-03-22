if get_option('gtk-doc')
  tumbler_doc_name = 'tumbler'

  conf_data = configuration_data()
  conf_data.set_quoted('PACKAGE_NAME', tumbler_doc_name)
  conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
  conf_data.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(tumbler_doc_name, meson.project_version()))

  configure_file(
    input: 'gtkdocentities.ent.in',
    output: 'gtkdocentities.ent',
    configuration: conf_data,
  )

  configure_file(
    input: 'version.xml.in',
    output: 'version.xml',
    configuration: configuration_data({
      'VERSION': meson.project_version(),
    })
  )

  configure_file(
    configuration: configuration_data({
      'COPYRIGHT_YEAR': copyright_year,
    }),
    input: 'tumbler-docs.xml.in',
    output: 'tumbler-docs.xml',
    install: false,
  )

  gnome.gtkdoc(
    tumbler_doc_name,
    src_dir: 'tumbler',
    ignore_headers: [
      'tumbler-visibility.h',
      'tumbler-marshal.h',
    ],
    main_xml: meson.project_build_root() / 'docs' / 'reference' / 'tumbler' / 'tumbler-docs.xml',
    dependencies: tumbler_dep,
    gobject_typesfile: 'tumbler.types',
    install: true,
    check: false,
  )
endif
