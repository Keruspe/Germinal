<a href="https://scan.coverity.com/projects/germinal">
  <img alt="Coverity Scan Build Status"
       src="https://scan.coverity.com/projects/6315/badge.svg"/>
</a>

Germinal is a minimalist vte-based terminal emulator.

Screenshot available here: <http://www.imagination-land.org/images/Germinal/Germinal-1.png>

You will need tmux for Germinal to work out of the box.
You need to add `new-session` in your `~/.tmux.conf` so that tmux attach creates a new one if none exist.

To configure it, open dconf-editor and go into /org/gnome/Germinal.
By default, germinal launches tmux, but you can specify a custom command by adding extra args to the commandline (no
need for quoting, example: `germinal /bin/bash -l` will execute `/bin/bash -l` instead of tmux).
By default, germinal uses dconf to store its configuration, but it will use `~/.config/germinal/settings` instead if it
exists. Configuration is in standard keyfile `ini` format, in a `[Germinal]` section.

List of available keyboard actions:

    - `<Ctrl><+>`          -> Zoom
    - `<Ctrl><->`          -> De-Zoom
    - `<Ctrl><0>`          -> Reset Zoom
    - `<Ctrl><Shift><C>`   -> Copy
    - `<Ctrl><Shift><V>`   -> Paste
    - `<Ctrl><Shift><Q>`   -> Quit
    - `<Ctrl><Shift><O>`   -> Split window (like terminator)
    - `<Ctrl><Shift><E>`   -> Split window (like terminator)
    - `<Ctrl><Tab>`        -> Next tab
    - `<Ctrl><Shift><Tab>` -> Previous tab
    - `<Ctrl><Shift><T>`   -> New tab
    - `<Ctrl><Shift><N>`   -> Next pane
    - `<Ctrl><Shift><P>`   -> Previous pane
    - `<Ctrl><Shift><W>`   -> Close current pane
    - `<Ctrl><Shift><X>`   -> Resize current pane

To build it, go into the Germinal directory

```
./autogen.sh
./configure --sysconfdir=/etc
make
sudo make install
sudo glib-compile-schemas /usr/share/glib-2.0/schemas/
```

You can see more information [here](http://www.imagination-land.org/posts/2015-01-31-germinal-7-released.html).

Latest release is [Germinal 26](http://www.imagination-land.org/posts/2019-10-07-germinal-26-released.html).

Direct link to download: <http://www.imagination-land.org/files/germinal/germinal-26.tar.xz>
