#!/usr/bin/python3

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-python/extensions',
        [
            'src/nemo-emblems.py'
        ]
    )
]

DistUtilsExtra.auto.setup(
    name         = "nemo-emblems",
<<<<<<< HEAD
    version      = "3.2.0",
    description  = "Nemo extension for adding emblems to items.",
=======
    version      = "3.0.0",
    description  = "Change your folder and file emblems",
>>>>>>> upstream/master
    author       = "Linux Mint",
    author_email = "root@linuxmint.com",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
