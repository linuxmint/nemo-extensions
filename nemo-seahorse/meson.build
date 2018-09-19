project('nemo-seahorse',
  'c',
  version: '3.8.0',
  meson_version: '>=0.40.0'
)
project_url = 'https://github.com/linuxmint/nemo-extensions'


gnome     = import('gnome')
pkgconfig = import('pkgconfig')
i18n      = import('i18n')

conf = configuration_data()

################################################################################
# Nemo-extensions generic

gmodule   = dependency('gmodule-2.0',       version: '>=2.14.0')
libnemoext= dependency('libnemo-extension', version: '>=1.0.0')

nemo_extensiondir = libnemoext.get_pkgconfig_variable('extensiondir')
nemo_libdir       = libnemoext.get_pkgconfig_variable('libdir')
nemo_extensionver = libnemoext.version().split('.')
conf.set_quoted('NEMO_EXTENSION_DIR', nemo_extensiondir)
conf.set_quoted('NEMO_LIBDIR',        nemo_libdir)

conf.set('NEMO_VERSION_MAJOR', nemo_extensionver[0])
conf.set('NEMO_VERSION_MINOR', nemo_extensionver[1])
conf.set('NEMO_VERSION_MICRO', nemo_extensionver[2])

################################################################################
# Dependencies

gtk3 = dependency('gtk+-3.0')
gio  = dependency('gio-2.0')

gnome_keyring = dependency('gnome-keyring-1')
dbus_glib = dependency('dbus-glib-1', version: '>=0.78')
cryptui = dependency('cryptui-0.0')
gcr = dependency('gcr-3', version: '>=3.4.0')

if get_option('libnotify')
  libnotify = dependency('libnotify', version: '>=0.7.0')
else
  libnotify = dependency('', required: false)
endif
conf.set('HAVE_LIBNOTIFY', libnotify.found())

conf.set('WITH_SHARING', true)

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

gettext_package = meson.project_name()
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)
conf.set_quoted('GNOMELOCALEDIR',  join_paths(
  get_option('prefix'), get_option('datadir'), 'locale'))

conf.set_quoted('PKGDATADIR', join_paths(
  get_option('prefix'), get_option('datadir'), meson.project_name()))
conf.set_quoted('DATA_DIR', join_paths(
  get_option('prefix'), get_option('datadir')))

conf.set_quoted('SEAHORSE_UIDIR', join_paths(
  get_option('prefix'), get_option('datadir'), 'nemo-seahorse', 'ui'
))

conf.set('GCR_API_SUBJECT_TO_CHANGE', true)
conf.set('LIBCRYPTUI_API_SUBJECT_TO_CHANGE', true)


configure_file(
  output: 'config.h',
  configuration: conf,
)

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')
rootInclude = include_directories('.')

subdir('po')
subdir('data')
subdir('nemo-ext')
subdir('tool')
