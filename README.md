Germinal is a minimalist vte-based terminal emulator.
It won't evolve much since it's just a small project inspired by Sakura to fit my needs

You need glib 2.28, gtk+ 3, vte 2.91 and tmux.
You also need to add "new-session" at the end of your .tmux.conf so that tmux attach creates a new one if none exist.

To configure it, open dconf-editor and go into /org/gnome/Germinal
By default, germinal launches tmux, but you can specify a custom command by adding extra args to the commandline (no
need for quoting, example: `germinal /bin/bash -l` will execute `/bin/bash -l` instead of tmux).

To build it, go into the Germinal directory

```
./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc
make
sudo make install
```

You can see more information [here](http://www.imagination-land.org/posts/2015-01-31-germinal-7-released.html).

Latest release is [Germinal 11](http://www.imagination-land.org/posts/2015-04-03-germinal-11-released.html).

Direct link to download: <http://www.imagination-land.org/files/germinal/germinal-11.tar.xz>
