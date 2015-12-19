## Nemo Compare

**Nemo extension for file comparison via context menu.**

This handy extension adds context menu options to Nemo when one or multiple
files are selected. This way, you can compare the files directly with your
favorite comparison engine (e.g. Meld or Diffuse) or mark files for later
comparison. It auto-detects the type of comparison and runs the approriate
engine capable for that amount of diffs. It distinguishes between
 * normal diff,
 * three-way diff and
 * multi diff.

### Installation

The preferred installation method is via your system's package manager.
Manual installation can be done by creating a symlink to the main script
in `nemo-python`'s extension lookup paths which are by default:
`/usr/share/nemo-python/extensions/` or
`~/.local/share/nemo-python/extensions/`.
Restart `nemo` after that.

### Configuration

`nemo-compare` comes with the preferences tool `nemo-compare-preferences`
which lets you chose which of your installed engines shall be called on
what kind of comparison.

### Latest changes

`nemo-compare` is now written in Python 3 and accesses GTK+ through PyGObject.
Also, multiple files can now be remembered and compared with multiple other
files.

### Questions? Issues?

Don't hesitate to ask and report bugs.
