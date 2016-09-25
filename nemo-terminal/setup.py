#!/usr/bin/python3

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-python/extensions',
        [
            'src/nemo-terminal.py'
        ]
    ),
    (
        '/usr/share/glib-2.0/schemas',
        [
            'src/org.nemo.extensions.nemo-terminal.gschema.xml'
        ]
    )
]

DistUtilsExtra.auto.setup(
    name         = "nemo-terminal",
    version      = "3.2.0",
    description  = "Nemo extension for embedding a terminal into it.",
    author       = "Linux Mint",
    author_email = "Fabien Loison <flo@flogisoft.com>",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
