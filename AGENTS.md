# AGENTS.md

This file provides guidance to Claude Code (claude.ai/code) and other AI coding agents when working with code in this repository. **Always read this file at the start of every session.** Prefer `AGENTS.md` over `CLAUDE.md` when both are present.

## Project overview

Germinal is a minimalist GTK4/libadwaita terminal emulator written in C (gnu99). It wraps VTE (the GNOME terminal widget) and delegates all tab and pane management to tmux. The default startup command is `tmux -u2 new -As0`.

## Build commands

```sh
meson setup _build          # configure (run once)
ninja -C _build             # compile
sudo ninja -C _build install

ninja -C _build test        # run tests
```

GSettings schemas must be compiled before running from the build tree. `gnome.post_install(glib_compile_schemas: true)` handles this on install; for local testing, run `glib-compile-schemas _build/data/gsettings/`.

RPM packaging via mock:
```sh
./mock-build.sh                   # defaults to fedora-rawhide-x86_64
./mock-build.sh fedora-41-x86_64  # specific chroot
```

## Architecture

The application is a single `AdwApplication` that creates one window per invocation. The three main GObject types are:

- **`GerminalSettings`** (`germinal-settings.c/h`) — thin wrapper around `GSettings`. Transparently switches between a dconf backend (default) and a keyfile backend at `~/.config/germinal/settings` when that file exists. Settings keys are `#define`d in `germinal-settings.h`.

- **`GerminalTerminal`** (`germinal-terminal.c/h`) — subclass of `VteTerminal`. Owns settings, handles all keyboard shortcuts (key-pressed controller at `GTK_PHASE_CAPTURE`), trackpad/mouse scroll-to-zoom (reads `org.gnome.desktop.peripherals.*` for natural-scroll state), URL regex matching, clipboard operations, and font/color/scrollback live updates via `GSignalGroup` on settings changes. All tmux commands (splits, tab management, pane navigation) are fired by `germinal_terminal_spawn()` from the key handler.

- **`GerminalWindow`** (`germinal-window.c/h`) — subclass of `AdwApplicationWindow`. Contains the terminal widget, builds the right-click `GtkPopoverMenu` with actions registered via `g_action_map_add_action_entries`, manages the `decorated` setting, and defers the initial command spawn via `g_idle_add_full` until both window and terminal are realized. Also owns the in-buffer search bar (`GtkRevealer` + `GtkSearchEntry`), opened with Ctrl+F and closed with Escape.

Entry point (`germinal.c`) creates a terminal, creates a window around it, presents the window maximized, then triggers `germinal_window_spawn_command`. Command-line arguments beyond flags become the startup command, overriding the GSettings value.

## Memory management

Always use GLib automatic memory management. Apply to every C file touched, not only the file under edit.

| Macro | Use when |
|---|---|
| `g_autofree` | Plain heap allocation: `g_strdup`, `g_malloc`, `g_new` for non-GObject structs |
| `g_autoptr(Type)` | GObject-derived or boxed types with a registered cleanup (`GError`, `GFile`, `GMenu`, `GSimpleActionGroup`, `PangoFontDescription`, `VteRegex`, …) |
| `g_auto(Type)` | Stack-allocated types with a cleanup function: `GStrv`, … |

Key rules:
- Replace `g_free(old); ptr = g_strdup(new)` with `g_set_str(&ptr, new)` (GLib ≥ 2.76, project requires ≥ 2.76).
- Use `g_steal_pointer(&ptr)` when transferring ownership out of an auto-managed variable (e.g. passing a `g_autofree`/`g_autoptr` local to a function that takes ownership).
- In `dispose`: use `g_clear_object`, `g_clear_pointer`, `g_clear_handle_id` — they null the pointer, making double-dispose safe. Release all reference-counted objects here (GObject, GSettings, GSignalGroup, GMenu, …), not in `finalize`.
- In `finalize`: use `g_free` for plain heap allocations (`gchar *`, `guint *`, plain structs). Do **not** call `g_object_unref` or other ref-counting unrefs here — those belong in `dispose`.
- Do **not** use auto-cleanup on a variable whose ownership is intentionally transferred — use `g_steal_pointer` to make the handoff explicit.

## Preparing a release

To cut a new release (version N), update all five files in one pass:

1. **`meson.build`** — bump `version: 'N-1'` → `'N'`
2. **`NEWS`** — prepend a new `NEW in N (DD/MM/YYYY)` block with user-facing changes
3. **`data/metainfo/org.gnome.Germinal.metainfo.xml.in`** — prepend a new `<release version="N" type="stable" date="YYYY-MM-DD">` element inside `<releases>`
4. **`germinal.spec`** — bump `Version:` and prepend a new `%changelog` entry (`* Www Mon DD YYYY Name <email> - N-1`)
5. **`README.md`** — update the "Latest release" link and tarball URL, and keep the keyboard shortcuts table in sync with any new shortcuts

Build (`ninja -C _build`) to confirm the version string compiles correctly. Then run `./release.sh N`, which validates the metainfo, rebuilds, updates translations, commits, builds the dist tarball, and creates the git tag.

On Fedora, also build and install the RPM:

```sh
./mock-build.sh fedora-$(rpm -E '%{fedora}')-$(uname -m)
sudo dnf install --nogpgcheck _build/mock-result/germinal-N-1.*.rpm
```

## Code conventions

- Standard GLib/GObject C patterns throughout: `G_DEFINE_TYPE_WITH_PRIVATE`, `G_GNUC_UNUSED` on unused parameters.
- Prefer `GSignalGroup` over plain `g_signal_connect` whenever connecting signals to a GObject whose lifetime is managed separately (e.g. a settings object, a terminal widget, a search bar). Group all signals for the same target into one `GSignalGroup` — create the group, call `g_signal_group_connect` for each signal, then call `g_signal_group_set_target`. Release the group with `g_clear_object` in `dispose`. Plain `g_signal_connect` is acceptable only for controllers/gestures passed to `gtk_widget_add_controller` (owned and destroyed by the widget) and for `GSimpleAction` callbacks registered via `g_action_map_add_action_entries`.
- Register context-menu actions with `g_action_map_add_action_entries` and a static `GActionEntry` array using designated initialisers (`.name =`, `.activate =`) to avoid `-Wmissing-field-initializers` warnings from the `padding` field.
- Version-gating macros (`GLIB_VERSION_MIN_REQUIRED`, `GDK_VERSION_MAX_ALLOWED`, etc.) are set in `meson.build` to the minimum required versions — stay within those API bounds.
- All translatable strings use `_()` / `N_()` from `<glib/gi18n-lib.h>`; the gettext domain is `Germinal`.
- The only test (`tests/regexp/test-regexp.c`) validates the URL regex defined in `germinal-util.h`.
- When updating source files, keep `AGENTS.md` up to date to reflect any new patterns or architectural decisions.
