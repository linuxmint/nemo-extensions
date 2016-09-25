## Nemo Pastebin

**Nemo extension for pasting files to a pastebin service.**

**Nemo Pastebin** enables uploading files to a pastebin service via context menu.
Using `pastebinit`, is has support for various online services. The process
has visual feedback through notifications. On success, the pastebin URL is
copied to the clipboard and opened in the browser.

The extension's behaviour can be adjusted with the easy-to-use configuration tool
`nemo-pastebin-preferences`.

### Installation

The preferred installation method is via your system's package manager.
Manual installation can be done by creating a symlink to the main script
in `nemo-python`'s extension lookup paths which are by default:
`/usr/share/nemo-python/extensions/` or
`~/.local/share/nemo-python/extensions/`.
Restart `nemo` after that.

### Latest changes

`nemo-pastebin` is now written in Python 3 and supports actions in the
notifications. The preferences tool has more options and we have better error
mitigations.

### Questions? Issues?

Don't hesitate to ask and report bugs.
