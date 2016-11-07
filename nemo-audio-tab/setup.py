#!/usr/bin/env python


import os
import sys

import DistUtilsExtra.auto

# Create data files
data = [ ('/usr/share/nemo-python/extensions', ['nemo-extension/nemo-audio-tab.py', 'nemo-extension/nemo-audio-tab.glade']) ]

# Setup stage
DistUtilsExtra.auto.setup(
    name         = "nemo-audio-tab",
    version      = "3.2.0",
    description  = "View audio tag information from the file manager's properties tab",
    author       = "Linux Mint",
    author_email = "root@linuxmint.com",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",
    data_files   = data
    )

