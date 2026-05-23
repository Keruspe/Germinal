# Germinal

[![CI](https://github.com/Keruspe/Germinal/actions/workflows/ci.yml/badge.svg)](https://github.com/Keruspe/Germinal/actions/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Latest release](https://img.shields.io/github/v/tag/Keruspe/Germinal?label=release)](https://github.com/Keruspe/Germinal/releases)
[![Packaging status](https://repology.org/badge/tiny-repos/germinal.svg)](https://repology.org/project/germinal/versions)

Germinal is a minimalist terminal emulator based on [VTE](https://gitlab.gnome.org/GNOME/vte) and [libadwaita](https://gitlab.gnome.org/GNOME/libadwaita).

## Requirements

Germinal relies on [tmux](https://github.com/tmux/tmux) for tab and pane management. Add the following to `~/.tmux.conf` so that attaching creates a new session when none exists:

```
new-session
```

## Configuration

Germinal has a built-in preferences dialog, accessible from the header bar (gear icon) or the right-click context menu.

Settings are also available directly via GSettings under `/org/gnome/Germinal/`, using `dconf-editor` or the `gsettings` command-line tool.

If the file `~/.config/germinal/settings` exists, Germinal uses it as a keyfile backend instead of dconf. The format is standard `ini` with a `[Germinal]` section:

```ini
[Germinal]
font=Monospace 12
backcolor=#1c1c1c
forecolor=#c8c8c8
```

A custom startup command can be passed directly on the command line — no quoting needed:

```sh
germinal /bin/bash -l
```

## Keyboard shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl` `+` | Zoom in |
| `Ctrl` `-` | Zoom out |
| `Ctrl` `0` | Reset zoom |
| `Ctrl` `Shift` `C` | Copy |
| `Ctrl` `Shift` `V` | Paste |
| `Ctrl` `Shift` `Q` | Quit |
| `Ctrl` `Shift` `O` | Split pane vertically |
| `Ctrl` `Shift` `E` | Split pane horizontally |
| `Ctrl` `Tab` | Next tab |
| `Ctrl` `Shift` `Tab` | Previous tab |
| `Ctrl` `Shift` `T` | New tab |
| `Ctrl` `Shift` `N` | Next pane |
| `Ctrl` `Shift` `P` | Previous pane |
| `Ctrl` `Shift` `W` | Close current pane |
| `Ctrl` `Shift` `X` | Zoom current pane (tmux) |
| `Ctrl` `F` | Open/focus search bar |
| `Ctrl` `G` / `Enter` | Next search match |
| `Ctrl` `Shift` `G` | Previous search match |
| `Escape` | Close search bar |

## Mouse

- **Right-click** — opens the context menu (copy, paste, zoom, URL actions)
- **Shift + left-click** — opens the URL under the cursor
- **Ctrl + scroll** — zoom in/out

## Building

Germinal uses [Meson](https://mesonbuild.com):

```sh
meson setup _build
ninja -C _build
sudo ninja -C _build install
```

## Latest release

[Germinal 28](https://www.imagination-land.org/posts/2026-05-22-germinal-28-released.html) — [download tarball](https://www.imagination-land.org/files/germinal/germinal-28.tar.xz)

More background on the project: [Germinal 7 release post](https://www.imagination-land.org/posts/2015-01-31-germinal-7-released.html).

## RPM packaging

To build an RPM using [mock](https://github.com/rpm-software-management/mock):

```sh
./mock-build.sh                   # defaults to fedora-rawhide-x86_64
./mock-build.sh fedora-41-x86_64  # specific chroot
```

The script builds the Meson dist tarball, creates an SRPM, and rebuilds it inside the mock chroot. Results land in `_build/mock-result/`.
