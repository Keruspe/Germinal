test_regexp = executable('test-regexp',
  'regexp/test-regexp.c',
  dependencies:        [glib_dep, gtk_dep],
  include_directories: include_directories('../src/germinal'),
)
test('regexp', test_regexp)

test_settings = executable('test-settings',
  ['settings/test-settings.c', '../src/germinal/germinal-settings.c'],
  dependencies:        [glib_dep, gio_dep, gtk_dep],
  include_directories: include_directories('../src/germinal'),
)
test('settings', test_settings,
  env: ['GSETTINGS_SCHEMA_DIR=' + (meson.project_build_root() / 'data')],
)

test_palette_editor = executable('test-palette-editor',
  ['palette-editor/test-palette-editor.c',
   '../src/germinal/germinal-palette-editor.c',
   '../src/germinal/germinal-settings.c'],
  dependencies:        [glib_dep, gio_dep, gtk_dep],
  include_directories: include_directories('../src/germinal'),
)
test('palette-editor', test_palette_editor,
  env: ['GSETTINGS_SCHEMA_DIR=' + (meson.project_build_root() / 'data')],
)
