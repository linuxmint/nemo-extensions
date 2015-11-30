#!/usr/bin/python3
"""nemo-compare -- Nemo extension for file comparison via context menu.

Copyright (C) 2011  Guido Tabbernuk <boamaod@gmail.com>
Copyright (C) 2015  Eduard Dopler <kontakt@eduard-dopler.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

import gettext
import locale
import os
import signal
import sys
from urllib.parse import unquote

from gi.repository import GLib, GObject, Gio, Nemo

sys.path.append("/usr/share/nemo-compare")
import utils


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


TEXTDOMAIN = "nemo-compare"


def init_locale():
    """Initialize internationalization."""
    locale.setlocale(locale.LC_ALL, "")
    gettext.bindtextdomain(TEXTDOMAIN)
    gettext.textdomain(TEXTDOMAIN)


def _(message):
    """Return translated version of message.

    As the textdomain could have been changed by other (nemo-extension)
    scripts, this function checks whether it is set correctly.
    """
    if gettext.textdomain() != TEXTDOMAIN:
        init_locale()
    return gettext.gettext(message)


# extension's name and short description
NAME_DESC = "Nemo Compare:::" + _("File comparison via context menu.")


class NemoCompareExtension(GObject.GObject,
                           Nemo.MenuProvider,
                           Nemo.NameAndDescProvider):

    """Class adding context menu items for file comparison in Nemo.

    Files/Directories can be compared depending on the amount (normal,
    3way, multi comparision) or remembered for later comparison.
    """

    def __init__(self):
        """Load saved configuration file.

        At this time it is guaranteed to be up-to-date and that all
        engines there are present on this machine.
        """
        self.config = utils.NemoCompareConfig()
        self.remembered = []

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [NAME_DESC]

    def menu_activate_cb(self, menu, paths, remember=False):
        """Callback for menu item activation.

        If remembered is True, store the paths in self.remembered.
        Else lookup the comparator engine for that amount of paths and
        run it. The engine list is updated before.
        """
        # remember only
        if remember:
            self.remembered = paths
            return

        # check for engine changes
        self.config.update_engines()
        # start comparison (if engine for that amount is still available)
        engine_cmd = [utils.get_engine_for_amount(self.config, len(paths))]
        engine_cmd.extend(paths)

        if engine_cmd:
            flags = GLib.SpawnFlags.DEFAULT | GLib.SpawnFlags.SEARCH_PATH
            GLib.spawn_async(argv=engine_cmd, flags=flags)

    def get_file_items(self, window, files):
        """Create menu items for file comparison.

        Always create "Compare later" menu item.
        Create "Compare these files" if more than one file is selected.
        Create "Compare this/these file/s to remembered" if there is
        something remembered.
        The menu items' callback functions are connected with
        menu_activate_cb.
        """
        # handle only up to 10 selected files
        if len(files) > 10:
            return

        paths = [unquote(f.get_uri()[7:])
                 for f in files
                 if valid_file(f)]
        if not paths:
            return

        num_paths = len(paths)
        num_remembered = len(self.remembered)
        combined = paths + self.remembered
        num_combined = len(combined)

        menus = []
        # create "Compare within" if selection >=2
        if ((num_paths >= 2 and
             self.config.can_compare(num_paths))):
            item = Nemo.MenuItem(name="NemoCompareExtension::CompareWithin",
                                 label=_("Compare these items"),
                                 tip=_("Compare selected items"))
            item.connect("activate", self.menu_activate_cb, paths)
            menus.append(item)
        # create "Compare with remembered" if sth. is remembered
        if ((num_remembered >= 1 and
             self.config.can_compare(num_combined))):
            if num_remembered == 1:
                compare_to = os.path.relpath(self.remembered[0],
                                             os.path.dirname(paths[0]))
            else:
                compare_to = _("%s remembered items") % len(self.remembered)
            item = Nemo.MenuItem(name="NemoCompareExtension::CompareTo",
                                 label=_("Compare to: %s") % compare_to,
                                 tip=_("Compare with remembered items(s)"))
            item.connect("activate", self.menu_activate_cb, combined)
            menus.append(item)

        # always create "Compare later" menu item
        item = Nemo.MenuItem(name="NemoCompareExtension::CompareLater",
                             label=_("Compare Later"),
                             tip=_("Remember item(s) for later comparison"))
        item.connect("activate", self.menu_activate_cb, paths, True)
        menus.append(item)

        return menus

    def get_background_items(self, window, item):
        """Return empty list. Do not react on background clicks."""
        return []


def valid_file(file_object):
    """Return whether an file_object is comparable.

    That is either a regular file, a symbolic link or a directory,
    all with an "file" URI scheme.
    """
    valid_file_types = (Gio.FileType.DIRECTORY,
                        Gio.FileType.REGULAR,
                        Gio.FileType.SYMBOLIC_LINK)
    return ((file_object.get_uri_scheme() == "file" and
             file_object.get_file_type() in valid_file_types))
