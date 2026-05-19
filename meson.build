project(
  'germinal', 'c',
  version:         '26',
  license:         'GPL-3.0-or-later',
  meson_version:   '>= 1.1',
  default_options: ['c_std=gnu99', 'warning_level=2'],
)

i18n  = import('i18n')
gnome = import('gnome')

glib_req_version    = '2.76'
gtk_req_version     = '4.8'
vte_req_version     = '0.78.0'
adw_req_version     = '1.0'
gsd_req_version     = '3.28.1'

glib_major = glib_req_version.split('.')[0].to_int()
glib_minor = glib_req_version.split('.')[1].to_int()
gtk_major  = gtk_req_version.split('.')[0].to_int()
gtk_minor  = gtk_req_version.split('.')[1].to_int()
adw_major  = adw_req_version.split('.')[0].to_int()
adw_minor  = adw_req_version.split('.')[1].to_int()

if glib_minor % 2 == 1
  glib_minor = glib_minor + 1
endif
if gtk_minor % 2 == 1
  gtk_minor = gtk_minor + 1
endif

glib_dep    = dependency('glib-2.0',              version: '>= ' + glib_req_version)
gio_dep     = dependency('gio-2.0',               version: '>= ' + glib_req_version)
gtk_dep     = dependency('gtk4',                  version: '>= ' + gtk_req_version)
vte_dep     = dependency('vte-2.91-gtk4',         version: '>= ' + vte_req_version)
adwaita_dep = dependency('libadwaita-1',          version: '>= ' + adw_req_version)
pango_dep   = dependency('pango')
pcre2_dep   = dependency('libpcre2-8')
dbus_dep    = dependency('dbus-1')
systemd_dep = dependency('systemd',               required: false)
dependency('gsettings-desktop-schemas',           version: '>= ' + gsd_req_version)

gettext_package = 'Germinal'

add_project_arguments(
  '-DG_LOG_USE_STRUCTURED=1',
  '-DGETTEXT_PACKAGE="@0@"'.format(gettext_package),
  '-DPACKAGE_STRING="Germinal @0@"'.format(meson.project_version()),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_@0@_@1@'.format(glib_major, glib_minor),
  '-DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_@0@_@1@'.format(glib_major, glib_minor),
  '-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
  '-DGDK_VERSION_MAX_ALLOWED=GDK_VERSION_@0@_@1@'.format(gtk_major, gtk_minor),
  '-DADW_VERSION_MIN_REQUIRED=ADW_VERSION_@0@_@1@'.format(adw_major, adw_minor),
  '-DADW_VERSION_MAX_ALLOWED=ADW_VERSION_@0@_@1@'.format(adw_major, adw_minor),
  language: 'c',
)

gnome.post_install(glib_compile_schemas: true)

subdir('po')
subdir('data')
subdir('src')
subdir('tests')
