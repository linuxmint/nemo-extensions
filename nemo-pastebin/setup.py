#!/usr/bin/python3

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-pastebin',
        [
            'src/nemo-pastebin.py',
            'src/nemo-pastebin-preferences.py',
            'src/pastebin_utils.py'
        ]
    ),
    (
        '/usr/share/applications',
        [
            'data/nemo-pastebin-preferences.desktop'
        ]
    )
]

DistUtilsExtra.auto.setup(
    name         = "nemo-pastebin",
    version      = "3.0.1",
    description  = "Nemo extension for pasting files to a pastebin service.",
    author       = "Alessio Treglia",
    author_email = "quadrispro@ubuntu.com",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
