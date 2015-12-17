#!/usr/bin/python3
"""nemo-pastebin -- Nemo extension for pasting files to a pastebin service.

Copyright (C) 2009-2010 Alessio Treglia <quadrispro@ubuntu.com>
Copyright (C) 2015      Eduard Dopler <kontakt@eduard-dopler.de>

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
from collections import OrderedDict

from gi.repository import Gtk

import pastebin_utils as pb_utils


TEXTDOMAIN = "nemo-pastebin"


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


class NemoPastebinPreferences(Gtk.Window):

    """Main class for a preferences dialog using GTK+ through PyGObject."""

    def __init__(self):
        """Load config and create UI."""
        self.config = pb_utils.NemoPastebinConfig(watch_config=False)

        Gtk.Window.__init__(self)
        self._build_gui()

    def _build_gui(self):
        """Create GUI."""
        icon = self.render_icon(Gtk.STOCK_PREFERENCES, Gtk.IconSize.DIALOG)
        self.set_icon(icon)
        self.set_resizable(False)
        self.set_title(_("Nemo Pastebin Extension Preferences"))
        self.set_border_width(15)

        grid = Gtk.Grid(
            orientation=Gtk.Orientation.VERTICAL,
            column_spacing=15,
            row_spacing=10)
        self.add(grid)

        # present pastebin services in a combobox
        # select currently active one
        label = Gtk.Label(label=_("Pastebin Service") + ":", halign=Gtk.Align.START)
        grid.add(label)
        combo_services = Gtk.ComboBoxText(name=pb_utils.KEY_SERVICE)
        for service in pb_utils.get_pastebin_services():
            combo_services.append_text(service)
        combo_services.set_id_column(0)
        combo_services.set_active_id(self.config[pb_utils.KEY_SERVICE])
        grid.attach_next_to(combo_services, label, Gtk.PositionType.RIGHT, 1, 1)
        # text fields
        text_fields = OrderedDict(
            (
                (pb_utils.KEY_AUTHOR, _("Author")),
                (pb_utils.KEY_USERNAME, _("Username")),
                (pb_utils.KEY_PASSWORD, _("Password")),
                (pb_utils.KEY_TITLE, _("Title")),
                (pb_utils.KEY_PERMATAG, _("Perma-Tag")),
                (pb_utils.KEY_JABBERID, _("Jabber ID"))
            )
        )
        for key, name in text_fields.items():
            label = Gtk.Label(label=name + ":", halign=Gtk.Align.START)
            grid.add(label)
            config_value = self.config[key]
            entry = Gtk.Entry(name=key, text=config_value)
            grid.attach_next_to(entry, label, Gtk.PositionType.RIGHT, 1, 1)
            # handle password text field
            if key == pb_utils.KEY_PASSWORD:
                entry.set_visibility(False)
                entry.set_input_purpose(Gtk.InputPurpose.PASSWORD)
        # check boxes
        chk_browser = Gtk.CheckButton(name=pb_utils.KEY_OPENBROWSER,
                                      label=_("Open in browser when finished"))
        open_browser = self.config[pb_utils.KEY_OPENBROWSER].title() == "True"
        chk_browser.set_active(open_browser)
        grid.attach_next_to(chk_browser, label, Gtk.PositionType.BOTTOM, 2, 1)
        chk_cliboard = Gtk.CheckButton(name=pb_utils.KEY_COPYURL,
                                       label=_("Copy pastebin URL to clipboard"))
        copy_url = self.config[pb_utils.KEY_COPYURL].title() == "True"
        chk_cliboard.set_active(copy_url)
        chk_cliboard.set_margin_top(10)
        chk_cliboard.set_margin_bottom(15)
        grid.attach_next_to(chk_cliboard, chk_browser, Gtk.PositionType.BOTTOM, 2, 1)
        # buttons
        btn_cancel = Gtk.Button(stock=Gtk.STOCK_CANCEL)
        btn_cancel.connect_object("clicked", Gtk.main_quit, self, None)
        grid.add(btn_cancel)
        btn_ok = Gtk.Button(stock=Gtk.STOCK_OK)
        btn_ok.connect_object("clicked", self.save_event, self, None)
        grid.attach_next_to(btn_ok, btn_cancel, Gtk.PositionType.RIGHT, 1, 1)

    def save_event(self, widget, event, data=None):
        """Save settings and quit program."""
        grid = self.get_child()
        for child in grid.get_children():
            key = child.get_name()
            if isinstance(child, Gtk.Entry):
                value = child.get_text()
            elif isinstance(child, Gtk.ComboBoxText):
                value = child.get_active_text()
            elif isinstance(child, Gtk.CheckButton):
                value = str(child.get_active())
            else:
                continue
            if not value:
                value = ""
            self.config[key] = value

        self.config.save()
        Gtk.main_quit()
        return False


if __name__ == "__main__":
    preferences = NemoPastebinPreferences()
    preferences.connect("delete-event", Gtk.main_quit)
    preferences.show_all()
    Gtk.main()
