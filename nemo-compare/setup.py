#!/usr/bin/python3

# Nemo Compare

from setuptools import setup

# Setup stage
setup( packages=[],
    name         = "nemo-compare",
    version      = "5.4.0",
    description  = "Context menu comparison extension for Nemo file manager",
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
    #                     'meld'],
    data_files   = [
        ('/usr/share/nemo-python/extensions', ['src/nemo-compare.py']),
        ('/usr/share/nemo-compare', ['src/nemo-compare-preferences.py', 'src/utils.py']),
        ('/usr/bin', ['src/nemo-compare-preferences'])
    ]
)
