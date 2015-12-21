#!/usr/bin/python3

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-compare',
        [
            'src/nemo-compare.py',
            'src/nemo-compare-preferences.py',
            'src/compare_utils.py'
        ]
    ),
    (
        '/usr/share/applications',
        [
            'data/nemo-compare-preferences.desktop'
        ]
    )
]

DistUtilsExtra.auto.setup(
    name         = "nemo-compare",
    version      = "3.0.1",
    description  = "Nemo extension for file comparison via context menu.",
    author       = "Eduard Dopler",
    author_email = "kontakt@eduard-dopler.de",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
