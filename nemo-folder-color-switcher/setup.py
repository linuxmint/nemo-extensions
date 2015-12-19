#!/usr/bin/python3

import os
import sys

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-python/extensions',
        [
            'src/nemo-folder-color-switcher.py'
        ]
    ),
    (
        '/usr/share/icons/hicolor/10x10/apps',
        [
            'icons/nemo-folder-color-switcher-aqua.png',
            'icons/nemo-folder-color-switcher-beige.png',
            'icons/nemo-folder-color-switcher-black.png',
            'icons/nemo-folder-color-switcher-blue.png',
            'icons/nemo-folder-color-switcher-brown.png',
            'icons/nemo-folder-color-switcher-cyan.png',
            'icons/nemo-folder-color-switcher-green.png',
            'icons/nemo-folder-color-switcher-grey.png',
            'icons/nemo-folder-color-switcher-orange.png',
            'icons/nemo-folder-color-switcher-pink.png',
            'icons/nemo-folder-color-switcher-purple.png',
            'icons/nemo-folder-color-switcher-red.png',
            'icons/nemo-folder-color-switcher-sand.png',
            'icons/nemo-folder-color-switcher-teal.png',
            'icons/nemo-folder-color-switcher-white.png',
            'icons/nemo-folder-color-switcher-yellow.png'
        ]
    )
]

DistUtilsExtra.auto.setup(
    name         = "nemo-folder-color-switcher",
    version      = "3.0.0",
    description  = "Nemo extension for changing folder colors",
    author       = "Linux Mint",
    author_email = "root@linuxmint.com",
    url          = "https://github.com/linuxmint/folder-color-switcher",
    license      = "GPL3",
    data_files   = data
)

