#!/usr/bin/python3

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-git',
        [
            'src/nemo-git.py',
            'src/gitinfo.sh'
        ]
    )
]

DistUtilsExtra.auto.setup(
    name         = "nemo-git",
    version      = "3.2.0",
    description  = "Nemo extension for git branch and file info.",
    author       = "Eduard Dopler",
    author_email = "kontakt@eduard-dopler.de",
    url          = "https://github.com/EduardDopler/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
