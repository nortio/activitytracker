# Simple automatic time tracker for GNOME

This extension tracks how much time you spend on your computer and which applications you use the most.

NOTE: this is very rudimental and not polished at all. It might not work properly.

To use this:

```bash
# installs npm packages and compiles the code
make

# installs the extension
make install
```

The extension will output every 10 seconds a CSV row containing the focused window and other details to a file (`$HOME/.local/share/activitytracker/log`) which can then be processed by `process.py` (you need to have Python 3 installed, use like this `./process.py <log file> <filters>` - filters file must exist but can be empty) which outputs something like this:

```
2024-10-30
	org.gnome.TextEditor                 1:01:58.999000
	firefox                              0:46:29.934000
	gnome-terminal-server                0:14:10.001000
	VSCodium                             0:08:30.002000
	steam                                0:07:10
	vesktop                              0:02:40.145000
	org.gnome.Nautilus                   0:02:19.919000
	org.strawberrymusicplayer.strawberry 0:00:29.905000
	app.devsuite.Ptyxis                  0:00:10.095000
Total: 2:23:59
```

It also supports filters for further categorization based on the window title. For example this filter:

```
firefox
	youtube: .* - YouTube.*
```

Will match any window with `firefox` (matched against wmclass, appid, or window title) and apply further categorization based on the regex. For example, applying this filter will result in this:

```
2024-10-30
	firefox                              0:33:59.934000
	org.gnome.TextEditor                 0:23:19.999000
	gnome-terminal-server                0:14:10.001000
	youtube                 	     0:12:30
	VSCodium                             0:08:30.002000
	steam                                0:07:10
	vesktop                              0:02:40.145000
	org.gnome.Nautilus                   0:02:19.919000
	org.strawberrymusicplayer.strawberry 0:00:29.905000
	app.devsuite.Ptyxis                  0:00:10.095000
Total: 1:45:20
```

As you can see, `youtube` is shown in a different category.
