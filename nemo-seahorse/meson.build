project('nemo-seahorse',
  'c',
  version: '4.8.0',
  meson_version: '>=0.49.0'
)

project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome     = import('gnome')
pkgconfig = import('pkgconfig')
i18n      = import('i18n')

config = configuration_data()

################################################################################
# Find libnemo-extensions

libnemo = dependency('libnemo-extension', version: '>=2.0.0')
libnemo_extension_dir = libnemo.get_pkgconfig_variable('extensiondir')
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

if get_option('libnotify')
    libnotify = dependency('libnotify', version: '>=0.7.0')
else
    libnotify = dependency('', required: false)
endif
config.set('HAVE_LIBNOTIFY', libnotify.found())

if get_option('sharing')
    avahi = dependency('avahi-glib')
else
    avahi = dependency('', required: false)
endif
config.set('WITH_SHARING', avahi.found())


if get_option('gpg-check')
    accepted_versions = [ '=1.2', '=1.4', '=2.0', '>=2.1', ]

    gpg = find_program('gpg', 'gpg2')
    gpg_ver = run_command(gpg, '--version').stdout().split(' ')[2]

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

libgpgme = meson.get_compiler('c').find_library('gpgme')
gpgme_config = find_program('gpgme-config')
gpgme_ver = run_command(gpgme_config, '--version').stdout().strip()
gpgme_ver_required = '>=1.2.0'
if not gpgme_ver.version_compare(gpgme_ver_required)
    error('GPGME version @0@ was found, @1@ is required.'.format(
        gpgme_ver, gpgme_ver_required))
endif

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