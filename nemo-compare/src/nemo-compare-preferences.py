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
from collections import OrderedDict

from gi.repository import Gtk

import compare_utils as cp_utils


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

# Translations for the combo box labels
# (These are listed here for the single purpose to be found by translation
# software as in the further code there are accessed mostly indirectly.)
(
    _("2way"),
    _("3way"),
    _("nway")
)


class NemoCompareExtensionPreferences(Gtk.Window):

    """Main class for a preferences dialog using GTK+ through PyGObject."""

    def __init__(self):
        """Load config and create UI."""
        # configuration
        self.config = cp_utils.NemoCompareConfig()
        # combo boxes
        self.combo_boxes = OrderedDict()
        self.combo_boxes["2way"] = None
        self.combo_boxes["3way"] = None
        self.combo_boxes["nway"] = None

        Gtk.Window.__init__(self)
        self._build_gui()
        self._insert_config_data()

    def _build_gui(self):
        """Create GUI."""
        icon = self.render_icon(Gtk.STOCK_PREFERENCES, Gtk.IconSize.DIALOG)
        self.set_icon(icon)
        self.set_resizable(False)
        self.set_title(_("Nemo Compare Extension Preferences"))
        self.set_border_width(15)

        grid = Gtk.Grid(
            orientation=Gtk.Orientation.VERTICAL,
            column_homogeneous=True,
            column_spacing=10,
            row_spacing=10)
        self.add(grid)

        for name in self.combo_boxes:
            label = Gtk.Label(label=_(name) + ":", halign=Gtk.Align.START)
            grid.add(label)
            self.combo_boxes[name] = Gtk.ComboBoxText()
            grid.attach_next_to(self.combo_boxes[name], label,
                                Gtk.PositionType.RIGHT, 1, 1)

        btn_cancel = Gtk.Button(stock=Gtk.STOCK_CANCEL)
        btn_cancel.connect_object("clicked", Gtk.main_quit, self, None)
        grid.attach(btn_cancel, 0, 3, 1, 1)
        btn_ok = Gtk.Button(stock=Gtk.STOCK_OK)
        btn_ok.connect_object("clicked", self.save_event, self, None)
        grid.attach_next_to(btn_ok, btn_cancel, Gtk.PositionType.RIGHT, 1, 1)

    def _insert_config_data(self):
        """Insert engines from config into combo boxes.

        The currently active engine for every type will be preselected.
        In every box "(none)" will be the second entry if there is an
        active engine or the first if not.
        """
        installed_engines = cp_utils.get_installed_engines()
        for engine_type in self.combo_boxes:
            current_engine = self.config[engine_type]
            # current, if any
            if current_engine:
                self.combo_boxes[engine_type].append_text(current_engine)
            # blank line
            self.combo_boxes[engine_type].append_text("")
            # others
            for engine in installed_engines[engine_type]:
                self.combo_boxes[engine_type].append_text(engine)
            # preselect
            self.combo_boxes[engine_type].set_active(0)

    def save_event(self, widget, event, data=None):
        """Save settings and quit program."""
        selections = [box.get_active_text()
                      for box in self.combo_boxes.values()]
        self.config["2way"] = selections[0]
        self.config["3way"] = selections[1]
        self.config["nway"] = selections[2]
        self.config.save()
        Gtk.main_quit()
        return False


if __name__ == "__main__":
    preferences = NemoCompareExtensionPreferences()
    preferences.connect("delete-event", Gtk.main_quit)
    preferences.show_all()
    Gtk.main()
