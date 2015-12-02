#!/usr/bin/python3
"""nemo-emblems -- Nemo extension for adding emblems to items.

Copyright (C) 2014  Linux Mint
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
import signal
import subprocess
from urllib.parse import unquote

from gi.repository import GObject, Gio, Gtk, Nemo


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


TEXTDOMAIN = "nemo-emblems"


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
NAME_DESC = "Nemo Emblems:::" + _("Add emblems to files and folders")
# internal name for this property page
PAGE_NAME = "NemoPython::emblems"
# title for this property page
TAB_LABEL = _("Emblems")
# file info attribute for emblems
METADATA_EMBLEMS = "metadata::emblems"

# emblem filtering
EMBLEM_PREFIX = "emblem-"
HIDE_EMBLEM_NAMES = (
    "emblem-desktop",
    "emblem-noread",
    "emblem-nowrite",
    "emblem-readonly",
    "emblem-shared",
    "emblem-synchronizing",
    "emblem-symbolic-link",
    "emblem-unreadable"
)
HIDE_EMBLEM_PREFIXES = (
    "emblem-dropbox",
    "emblem-ubuntu",
    "emblem-rabbitvcs"
)
HIDE_EMBLEM_SUFFIXES = (
    "-symbolic"
)


class EmblemPropertyPage(GObject.GObject,
                         Nemo.PropertyPageProvider,
                         Nemo.NameAndDescProvider):

    """Add a property page for adding emblems to files and folders in Nemo."""

    def __init__(self):
        """Preload current icon names and their display names.

        Sadly we cannot create the icons here already. This has to be
        done for every property page.
        """
        self.icon_names = {}
        default_icon_theme = Gtk.IconTheme.get_default()
        system_icons = default_icon_theme.list_icons(context="Emblems")
        for icon_name in system_icons:
            if all((icon_name.startswith(EMBLEM_PREFIX),
                    icon_name not in HIDE_EMBLEM_NAMES,
                    not icon_name.startswith(HIDE_EMBLEM_PREFIXES),
                    not icon_name.endswith(HIDE_EMBLEM_SUFFIXES))):
                # get emblem display name
                icon_info = default_icon_theme.lookup_icon(icon_name=icon_name,
                                                           size=32, flags=0)
                display_name = icon_info.get_display_name()
                if not display_name:
                    display_name = icon_name.lstrip(EMBLEM_PREFIX).title()

                self.icon_names[icon_name] = display_name

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [NAME_DESC]

    def get_property_pages(self, files):
        """Called when a property page might be created.

        For now, we react only in case of a single file/directory.
        The file data is gathered here and then passed to _build_gui.
        """
        if len(files) != 1:
            return
        nemo_file = files[0]
        if nemo_file.get_uri_scheme() != "file":
            return

        filename = unquote(nemo_file.get_uri()[7:])
        gio_file = Gio.File.new_for_path(filename)
        file_info = gio_file.query_info(METADATA_EMBLEMS, 0, None)
        file_emblem_names = file_info.get_attribute_stringv(METADATA_EMBLEMS)

        file_data = {
            "filename": filename,
            "gio_file": gio_file,
            "file_info": file_info,
            "emblem_names": file_emblem_names
        }

        return self._build_gui(file_data)

    def _build_gui(self, file_data):
        """Create the GUI for the property page holding the emblems.

        The emblems (retrieved during instantiation) will be aligned in
        a grid model, selectable via check box.
        file_data has to be a dict holding the "filename", the Gio.File
        instance ("gio_file"), the Gio.FileInfo ("file_info") and its
        currently applied emblems ("emblem_names"). It will be passed to
        the checkbox toggle event.
        """
        tab_label = Gtk.Label(TAB_LABEL)
        tab_label.show()

        tab = Gtk.ScrolledWindow()
        grid = Gtk.Grid(
            column_homogeneous=True,
            row_homogeneous=True,
            column_spacing=15,
            row_spacing=10,
            border_width=20,
            vexpand=True,
            hexpand=True)
        tab.add(grid)

        left = 0
        top = 0
        emblems = sorted(self.icon_names.items(), key=lambda x: x[1])
        for emblem_name, display_name in emblems:
            emblem = Gtk.Image.new_from_icon_name(emblem_name,
                                                  Gtk.IconSize.LARGE_TOOLBAR)
            checkbutton = Gtk.CheckButton(label=display_name,
                                          image=emblem,
                                          always_show_image=True)
            # check current active emblems
            if emblem_name in file_data["emblem_names"]:
                checkbutton.set_active(True)

            checkbutton.connect("toggled", self.on_button_toggled, emblem_name,
                                file_data)
            checkbutton.show()
            grid.attach(checkbutton, left, top, 1, 1)
            left += 1
            if left > 2:
                left = 0
                top += 1

        tab.show_all()
        return Nemo.PropertyPage(name=PAGE_NAME, label=tab_label, page=tab),

    def on_button_toggled(self, button, emblem_name, file_data):
        """Toggle the emblem "emblem_name" in the file of "file_data".

        file_data has to be a dict holding the "filename", the Gio.File
        instance ("gio_file"), the Gio.FileInfo ("file_info") and its
        currently applied emblems ("emblem_names").
        """
        if ((button.get_active() and
             emblem_name not in file_data["emblem_names"])):
            file_data["emblem_names"].append(emblem_name)
        else:
            file_data["emblem_names"].remove(emblem_name)

        emblems = list(file_data["emblem_names"])
        emblems.append(None)

        file_data["file_info"].set_attribute_stringv(METADATA_EMBLEMS, emblems)
        file_data["gio_file"].set_attributes_from_info(file_data["file_info"],
                                                       flags=0,
                                                       cancellable=None)

        # touch the file (to force Nemo to re-render its icon)
        try:
            subprocess.call(["touch", file_data["filename"]])
        except OSError:
            pass
