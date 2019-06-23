#!/usr/bin/python3

# Nemo Media Columns

from setuptools import setup

# Setup stage
setup(
    name         = "nemo-media-columns",
    version      = "4.2.0",
    description  = "Column provider for nemo to show additional metadata for media and image files",
    author       = "Linux Mint",
    author_email = "root@linuxmint.com",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",

    # See debian/control for install-depends - this is useless here, except as reference.
    # install_requires (and only works with python modules.  It's equally as bad to have
    # some deps here, then draw them into debian/control using {python3:Depends} and add
    # them to our non-python depends.

    # install_requires = ['gir1.2-nemo-3.0 >=3.8',
    #                     'python-nemo >=3.8',
    #                     'gir1.2-gexiv2-0.10',
    #                     'mutagen',
    #                     'pypdf2',
    #                     'pil',
    #                     'pymediainfo'],
    data_files   = [
        ('/usr/share/nemo-python/extensions', ['nemo-media-columns.py'])
    ]
)
