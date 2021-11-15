#!/usr/bin/python3

# Nemo Terminal

from setuptools import setup

# Setup stage
setup(
    name         = "nemo-terminal",
    version      = "5.2.0",
    description  = "Embedded VTE terminal for Nemo file manager",
    author       = "Linux Mint",
    author_email = "root@linuxmint.com",
    url          = "https://github.com/linuxmint/nemo-extensions",
    license      = "GPL3",

    # See debian/control for install-depends - this is useless here, except as reference.
    # install_requires (and only works with python modules.  It's equally as bad to have
    # some deps here, then draw them into debian/control using {python3:Depends} and add
    # them to our non-python depends.

    # install_requires = ['python-nemo (>= 1.0)',
    #                     'gir1.2-nemo-3.0',
    #                     'gir1.2-vte-2.91',
    #                     'gir1.2-gtk-3.0 (>= 3.8.4~)',
    #                     'gir1.2-glib-2.0 (>= 1.38.0~)',
    #                     'gir1.2-xapp-1.0 (>= 3.8.0)'],

    data_files = [
        ('/usr/share/nemo-python/extensions', ['src/nemo_terminal.py']),
        ('/usr/bin',                          ['src/nemo-terminal-prefs']),
        ('/usr/share/nemo-terminal',          ['src/nemo-terminal-prefs.py',
                                               'pixmap/logo_120x120.png']),
        ('/usr/share/glib-2.0/schemas',       ['src/org.nemo.extensions.nemo-terminal.gschema.xml'])
    ]
)
