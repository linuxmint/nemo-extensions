project('nemo-seahorse',
  'c',
  version: '5.8.0',
  meson_version: '>=0.56.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome     = import('gnome')
pkgconfig = import('pkgconfig')
i18n      = import('i18n')

config = configuration_data()

################################################################################
# Find libnemo-extensions

libnemo = dependency('libnemo-extension', version: '>=2.0.0')
libnemo_extension_dir = libnemo.get_variable(pkgconfig: 'extensiondir')
libnemo_extension_ver = libnemo.version().split('.')

config.set('NEMO_VERSION_MAJOR', libnemo_extension_ver[0])
config.set('NEMO_VERSION_MINOR', libnemo_extension_ver[1])
config.set('NEMO_VERSION_MICRO', libnemo_extension_ver[2])

glib = dependency('glib-2.0', version: '>=2.14.0')

################################################################################
# Extension dependencies

gtk3 = dependency('gtk+-3.0')
gio  = dependency('gio-2.0')

dbus_glib = dependency('dbus-glib-1', version: '>=0.78')
cryptui = dependency('cryptui-0.0')
gcr = dependency('gcr-3', version: '>=3.4.0')

libnotify = dependency('libnotify', version: '>=0.7.0', required: get_option('libnotify'))
config.set('HAVE_LIBNOTIFY', libnotify.found())

avahi = dependency('avahi-glib', required: get_option('sharing'))
config.set('WITH_SHARING', avahi.found())


if get_option('gpg-check')
    accepted_versions = [ '=1.2', '=1.4', '=2.0', '>=2.1', ]

    gpg = find_program('gpg', 'gpg2')
    gpg_ver = run_command([gpg, '--version'], check: true).stdout().split(' ')[2]

    accepted = false
    foreach ver : accepted_versions
        if gpg_ver.version_compare(ver)
            accepted = true
        endif
    endforeach
    if not accepted
        error('Appropriate version of GnuPG not found.')
    endif
endif

libgpgme = dependency('gpgme', version: '>=1.2.0')

################################################################################
# Generic stuff

config.set_quoted('GETTEXT_PACKAGE', 'nemo-extensions')

config.set_quoted('DATA_DIR',         get_option('prefix')/get_option('datadir'))
config.set_quoted('GNOMELOCALEDIR',   get_option('prefix')/get_option('datadir')/'locale')
config.set_quoted('PKGDATADIR',       get_option('prefix')/get_option('datadir')/meson.project_name())
config.set_quoted('SEAHORSE_UIDIR',   'PKGDATADIR'/'ui')

config.set('GCR_API_SUBJECT_TO_CHANGE', true)
config.set('LIBCRYPTUI_API_SUBJECT_TO_CHANGE', true)


add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
configure_file(
  output: 'config.h',
  configuration: config,
)

rootInclude = include_directories('.')

subdir('po')
subdir('data')
subdir('nemo-ext')
subdir('tool')
