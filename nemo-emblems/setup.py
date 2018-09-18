#!/usr/bin/python3

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


from setuptools import setup

# Setup stage
setup(
    name         = "nemo-emblems",
    version      = "3.9.0",
    description  = "Change your folder and file emblems",
    author       = "Linux Mint",
    author_email = "root@linuxmint.com",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",

    # See debian/control for install-depends - this is useless here, except as reference.
    # install_requires (and only works with python modules.  It's equally as bad to have
    # some deps here, then draw them into debian/control using {python3:Depends} and add
    # them to our non-python depends.

    # install_requires = ['gir1.2-nemo-3.0>=3.9',
    #                     'python-nemo >=3.9'],
    data_files   = [
        ('/usr/share/nemo-python/extensions', ['nemo-extension/nemo-emblems.py'])
    ]
)
