schema = 'org.gnome.Germinal.gschema.xml'

install_data(
  schema,
  install_dir: get_option('datadir') / 'glib-2.0' / 'schemas',
)

# Compile the schema into the build tree so the test suite (which points
# GSETTINGS_SCHEMA_DIR here) can find it without an install.
gnome.compile_schemas(depend_files: files(schema))
