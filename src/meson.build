sources = [
  'main.c',
  'page_advanced.c',
  'callbacks.c',
  'features_page.c',
  'interface.c',
  'interface_I.c',
  'interface_aux.c',
  'layers_combo.c',
  'notebook.c',
  'render.c',
  'preview.c',
  'io_functions.c',
  'altsizeentry.c',
  'altcoordinates.c',
]

headers = [
  'main.h',
  'callbacks.h',
  'features_page.h',
  'interface.h',
  'interface_I.h',
  'interface_aux.h',
  'layers_combo.h',
  'notebook.h',
  'page_advanced.h',
  'render.h',
  'preview.h',
  'io_functions.h',
  'altsizeentry.h',
  'altcoordinates.h',
  'compat.h',
  'main_common.h',
  'plugin-intl.h',
]

# Create the plugin executable
plugin = executable('gimp-lqr-plugin',
  sources,
  include_directories : [
    include_directories('.'),
    include_directories('..'),
  ],
  dependencies : [gimp_dep, gimpui_dep, lqr_dep],
  install : true,
  install_dir : bindir
) 