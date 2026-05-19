bindir = get_option('prefix') / get_option('bindir')
conf   = configuration_data({'bindir': bindir})

# GSettings schema
gschema = configure_file(
  input:         'gsettings/org.gnome.Germinal.gschema.xml.in',
  output:        'org.gnome.Germinal.gschema.xml',
  configuration: {'GETTEXT_PACKAGE': gettext_package},
  install:       true,
  install_dir:   get_option('datadir') / 'glib-2.0' / 'schemas',
)
gnome.compile_schemas(depend_files: gschema)

# Desktop file (two-pass: substitute @bindir@, then merge translations)
desktop_in = configure_file(
  input:         'desktop/org.gnome.Germinal.desktop.in.in',
  output:        'org.gnome.Germinal.desktop.in',
  configuration: conf,
)
i18n.merge_file(
  input:       desktop_in,
  output:      'org.gnome.Germinal.desktop',
  type:        'desktop',
  po_dir:      '../po',
  install:     true,
  install_dir: get_option('datadir') / 'applications',
)

# AppStream metadata
i18n.merge_file(
  input:       'metainfo/org.gnome.Germinal.metainfo.xml.in',
  output:      'org.gnome.Germinal.metainfo.xml',
  po_dir:      '../po',
  install:     true,
  install_dir: get_option('datadir') / 'metainfo',
)

# D-Bus session service
_dbus_service = configure_file(
  input:         'dbus/org.gnome.Germinal.dbus.in',
  output:        'org.gnome.Germinal.dbus.service',
  configuration: conf,
)
install_data(
  _dbus_service,
  install_dir: dbus_dep.get_variable('session_bus_services_dir'),
  rename:      'org.gnome.Germinal.service',
)

# systemd user unit (optional)
if systemd_dep.found()
  _systemd_unit = configure_file(
    input:         'systemd/org.gnome.Germinal.systemd.in',
    output:        'org.gnome.Germinal.systemd.service',
    configuration: conf,
  )
  install_data(
    _systemd_unit,
    install_dir: systemd_dep.get_variable('systemduserunitdir'),
    rename:      'org.gnome.Germinal.service',
  )
endif
