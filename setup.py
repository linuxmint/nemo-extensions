#!/usr/bin/env python

# Folder Color 0.0.11 - http://launchpad.net/folder-color
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
data = [ 
	('/usr/share/folder-color', ['folder-color/folder-color.py']),
	('/usr/share/nemo-python/extensions', ['nemo-extensions/nemo-folder-color.py']),
	('/usr/share/caja-python/extensions', ['caja-extensions/caja-folder-color.py']) ]

# Setup stage
DistUtilsExtra.auto.setup(
    name         = "folder-color",
    version      = "0.0.1",
    description  = "Change the color of your folders",
    author       = "Linux Mint",
    author_email = "root@linuxmint.com",
    url          = "https://github.com/linuxmint/folder-color",
    license      = "GPL3",
    data_files   = data
    )
