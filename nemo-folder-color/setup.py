#!/usr/bin/env python

# Folder Color 0.0.9 - http://launchpad.net/folder-color
# Copyright (C) 2012-2014 Marcos Alvarez Costales https://launchpad.net/~costales
#
# folder-color is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# Folder Color is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Folder Color; if not, see http://www.gnu.org/licenses 
# for more information.


import os
import sys

import DistUtilsExtra.auto

# Create data files
data = [ ('/usr/share/nemo-python/extensions', ['nemo-extension/nemo-folder-color.py']) ]

# Setup stage
DistUtilsExtra.auto.setup(
    name         = "nemo-folder-color",
    version      = "0.0.9",
    description  = "Change your folder color with just a click",
    author       = "Marcos Alvarez Costales https://launchpad.net/~costales",
    author_email = "https://launchpad.net/~costales",
    url          = "https://launchpad.net/folder-color",
    license      = "GPL3",
    data_files   = data
    )

