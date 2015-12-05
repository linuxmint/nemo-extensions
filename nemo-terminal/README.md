## Nemo Terminal

**Nemo extension for embedding a terminal into it.**

By creating a new pane in every Nemo window, **Nemo Terminal** gives
you access to an embedded terminal in your file browser.
It automatically follows the currently active directory in Nemo and has
support for pasting paths via drag and drop.
You can trigger the terminal pane by hotkey (default F4) and/or
configure it to be visible by default in its gconfig file.

### Installation

The preferred installation method is via your system's package manager.
Manual installation can be done by moving the extension in
`nemo-python`'s extension lookup paths which are by default:
`/usr/share/nemo-python/extensions/` or
`~/.local/share/nemo-python/extensions/`.
The XML configuration file has to be located in GLib's settings path:
`usr/share/glib-2.0/schemas`.
Recompile your schemas (`glib-compile-schemas`) and restart `nemo` after
that.

### Latest changes

`nemo-terminal` was ported to Python 3 and now runs the user's default
shell.

### Questions? Issues?

Don't hesitate to ask and report bugs.
