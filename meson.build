project('gimp-lqr-plugin', 'c',
  version : '0.8.0git',
  license : 'GPL-3.0-or-later',
  meson_version : '>= 0.51.0',
  default_options : [
    'c_std=c99',
    'warning_level=2',
  ]
)

# Version information
plugin_version = meson.project_version()
plugin_name = meson.project_name()

# Compiler flags
cc = meson.get_compiler('c')
add_project_arguments(
  '-DDEBUG',
  language : 'c'
)

# Dependencies
gimp_required_version = '>= 3.0.0'
gimp_dep = dependency('gimp-3.0', version : gimp_required_version)
gimpui_dep = dependency('gimpui-3.0', version : gimp_required_version)
lqr_dep = dependency('lqr-1', version : '>= 0.4.0')

# Gettext
i18n = import('i18n')
gettext_package = 'gimp30-lqr-plugin'

# Installation paths
gimp_libdir = gimp_dep.get_variable(pkgconfig : 'gimplibdir')
gimp_datadir = gimp_dep.get_variable(pkgconfig : 'gimpdatadir')
bindir = get_option('gimp_plugindir')
datadir = get_option('gimp_plugin_datadir')

if bindir == ''
  bindir = gimp_libdir / 'plug-ins' / plugin_name
endif
if datadir == ''
  datadir = gimp_datadir / 'plug-ins' / plugin_name
endif

message('Installation paths:')
message('bindir: ' + bindir)
message('datadir: ' + datadir)

# Configuration header
config_h = configuration_data()
config_h.set('ENABLE_NLS', 1)
config_h.set_quoted('GETTEXT_PACKAGE', gettext_package)
config_h.set('HAVE_BIND_TEXTDOMAIN_CODESET', 1)
config_h.set('HAVE_DCGETTEXT', 1)
config_h.set('HAVE_GETTEXT', 1)
config_h.set('HAVE_LC_MESSAGES', 1)
config_h.set_quoted('PACKAGE_NAME', plugin_name)
config_h.set_quoted('PACKAGE_VERSION', plugin_version)
config_h.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(plugin_name, plugin_version))
config_h.set_quoted('PACKAGE_TARNAME', plugin_name)
config_h.set_quoted('PLUGIN_NAME', plugin_name)
config_h.set_quoted('PLUGIN_VERSION', plugin_version)
config_h.set('PLUGIN_MAJOR_VERSION', 0)
config_h.set('PLUGIN_MINOR_VERSION', 8)
config_h.set_quoted('PLUGIN_MICRO_VERSION', '0git')
config_h.set('STDC_HEADERS', 1)
config_h.set_quoted('LOCALEDIR', bindir / 'locale')

# Check for headers
foreach h : ['inttypes.h', 'locale.h', 'stdint.h', 'stdio.h', 'stdlib.h',
             'strings.h', 'string.h', 'sys/stat.h', 'sys/types.h', 'unistd.h']
  config_h.set('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

configure_file(
  output : 'config.h',
  configuration : config_h
)

# Subdirectories
subdir('src')
subdir('batch')
subdir('po')
subdir('help') 
