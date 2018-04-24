#!/usr/bin/python2
# -*- coding: utf-8 -*-
# nemo-pastebin - Nemo extension to paste a file to a pastebin service
# Written by:
#    Alessio Treglia <quadrispro@ubuntu.com>
# Copyright (C) 2009-2010, Alessio Treglia
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this package; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#

import os
from distutils.core import setup
from DistUtilsExtra.command import *

def get_app_path_files():
    return [
        'data/nemo-pastebin.png',
        'data/nemo-pastebin-notification',
        'data/nemo-pastebin-configurator.ui'
    ]

pkg_data_files = [
    ('share/pixmaps/', ['data/nemo-pastebin.svg']),
    ('share/glib-2.0/schemas/', ['data/nemo-pastebin.gschema.xml']),
    ('share/nemo-pastebin/', get_app_path_files()),
]

pkg_scripts = [
    'nemo-pastebin.py',
    'nemo-pastebin-configurator.py'
]

pkg_short_dsc = "Nemo extension to send files to a pastebin"

pkg_long_dsc = """nemo-pastebin is a Nemo extension written in Python, which allows users to upload text-only files to a pastebin service just by right-clicking on them. Users can also add their favorite service just by creatine new presets."""

setup(name='nemo-pastebin',
      version='3.6.0',
      platforms=['all'],
      author='Linux Mint',
      author_email='root@linuxmint.com',
      license='GPL-2',
      url='https://github.com/linuxmint/nemo-extensions',
      description=pkg_short_dsc,
      long_description=pkg_long_dsc,
      data_files=pkg_data_files,
      scripts=pkg_scripts,
      cmdclass = { "build" : build_extra.build_extra,
                   "build_i18n" :  build_i18n.build_i18n,
                   "build_icons" :  build_icons.build_icons,
                   "clean": clean_i18n.clean_i18n,
                   }
      )
