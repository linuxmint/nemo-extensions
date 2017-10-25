#!/usr/bin/env python


import os
import sys

import DistUtilsExtra.auto

# Create data files
data = [ ('/usr/share/nemo-python/extensions', ['nemo-extension/nemo-pdf.py']) ]

# Setup stage
DistUtilsExtra.auto.setup(
    name         = "nemo-pdf",
    version      = "0.1",
    description  = "Tools for PDF files in Nemo",
    author       = "Mickael Albertus",
    author_email = "mickael.albertus@gmail.com",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
