# Nemo Extensions

**Extensions for Cinnamon's file manager Nemo.**


## Components & Status

The main component `nemo-python` provides Python scripts access to Nemo's
extension interface. It includes the possibility to create property pages
and custom columns. `nemo-python` passes the current view's file objects
to the script which does its magic and then decides how to present it to
the user.
Many useful extensions have already been written:

| Component                         | Current Status       | Refactored |
|-----------------------------------|----------------------|------------|
| ~~nemo-audio-tab~~ (replaced)     | see metadata-tab     | obsolete   |
| nemo-compare                      | functional (Py3)     | yes        |
| nemo-dropbox                      | functional (Py3+C)   | (C)        |
| nemo-emblems                      | functional (Py3)     | yes        |
| nemo-fileroller                   | functional (C)       | (C)        |
| nemo-folder-color-switcher        | functional (Py3)     | yes        |
| nemo-gtkhash                      | functional (C)       | (C)        |
| nemo-image-converter              | functional (C)       | (C)        |
| ~~nemo-media-columns~~ (replaced) | see metadata-columns | obsolete   |
| nemo-metadata-columns             | NEW (Py3)            | yes        |
| nemo-metadata-tab                 | NEW (Py3)            | yes        |
| nemo-pastebin                     | functional (Py3)     | yes        |
| nemo-preview                      | functional (C)       | (C)        |
| nemo-python                       | functional (C)       | (C)        |
| ~~nemo-rabbitvcs~~ (removed)      | removed              | obsolete   |
| nemo-repairer                     | functional (C)       | (C)        |
| nemo-seahorse                     | functional (C)       | (C)        |
| nemo-share                        | functional (C)       | (C)        |
| nemo-terminal                     | functional (Py3)     | yes        |


In the latest development all Python scripts have been ported to Python 3.
While there is still cleanup work to do, most scripts are functional.
Only `nemo-rabbitvcs` got lost on the way. Maybe we (you?) come up with
some replacement sometime.

**For extension descriptions** see their README files.


## Installation

The preferred installation method is via your system's package manager.
Manual installation can be done by moving your desired extension in
`nemo-python`'s extension lookup paths which are by default:
`/usr/share/nemo-python/extensions/` or
`~/.local/share/nemo-python/extensions/`.
Restart `nemo` after that.


## Configuration

Many extensions work out-of-the-box. Others come with handy configuration
tools. Check the extension's description for details. Settings are always
stored on a per-user basis.


## Roadmap

- [x] ~~Port to Python 3~~
- [x] ~~include nemo-emblems~~
- [x] ~~include folder-color-switcher~~
- [x] Refactor Python 3 code
- [ ] Update debian build files (WIP)


## Questions? Issues?

Don't hesitate to ask and report bugs.
