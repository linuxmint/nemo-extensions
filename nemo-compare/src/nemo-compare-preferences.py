#!/usr/bin/python
# -*- coding: utf-8 -*-
#    nemo-compare --- Context menu extension for Nemo file manager
#    Copyright (C) 2011  Guido Tabbernuk <boamaod@gmail.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

import gettext
import locale

from gi.repository import Gtk

import utils

def combo_add_and_select(combo, text):
    """Convenience function to add/select item in ComboBoxEntry."""
    combo.append_text(text)
    model = combo.get_model()
    iter = model.get_iter_first()
    while model.iter_next(iter):
        iter = model.iter_next(iter)
    combo.set_active_iter(iter)


class NemoCompareExtensionPreferences:
    """The main class for a preferences dialog using GTK+ through PyGObject."""

    combo_normal = None
    combo_3way = None
    combo_multi = None

    def cancel_event(self, widget, event, data=None):
        """Quit the program."""
        Gtk.main_quit()
        return False

    def changed_cb(self, combo):
        """On ComboBox change, change data accordingly."""
        selection = combo.get_active_text()

        if combo is self.combo_normal:
            self.config.diff_engine = selection
        elif combo is self.combo_3way:
            self.config.diff_engine_3way = selection
        elif combo is self.combo_multi:
            self.config.diff_engine_multi = selection

    def save_event(self, widget, event, data=None):
        """Save settings and quit program."""
        self.config.save()
        Gtk.main_quit()
        return False

    def __init__(self):
        """Load config and create UI."""
        self.config = utils.NemoCompareConfig()
        self.config.load()
        self.config.add_missing_predefined_engines()

        # initialize i18n
        locale.setlocale(locale.LC_ALL, "")
        gettext.bindtextdomain(utils.APP)
        gettext.textdomain(utils.APP)
        _ = gettext.gettext

        self.window = Gtk.Window()
        icon = self.window.render_icon(Gtk.STOCK_PREFERENCES,
                                       Gtk.IconSize.DIALOG)
        self.window.set_icon(icon)
        self.window.set_resizable(False)
        self.window.set_title(_("Nemo Compare Extension Preferences"))
        self.window.connect("delete_event", self.cancel_event)
        self.window.set_border_width(15)

        main_vbox = Gtk.VBox(False, 0)
        self.window.add(main_vbox)

        # normal diff
        frame = Gtk.Frame(label=_("Normal Diff"))
        self.combo_normal = Gtk.ComboBoxText()
        for text in self.config.engines:
            if text == self.config.diff_engine:
                combo_add_and_select(self.combo_normal, text)
            else:
                self.combo_normal.append_text(text)
        self.combo_normal.connect("changed", self.changed_cb)
        frame.add(self.combo_normal)
        main_vbox.pack_start(frame, True, True, 0)

        # 3-way diff
        frame_3way = Gtk.Frame(label=_("Three-Way Diff"))
        self.combo_3way = Gtk.ComboBoxText()
        for text in self.config.engines:
            if text == self.config.diff_engine_3way:
                combo_add_and_select(self.combo_3way, text)
            else:
                self.combo_3way.append_text(text)
        self.combo_3way.connect("changed", self.changed_cb)
        frame_3way.add(self.combo_3way)
        main_vbox.pack_start(frame_3way, True, True, 0)

        # n-way diff
        frame_multi = Gtk.Frame(label=_("N-Way Diff"))
        self.combo_multi = Gtk.ComboBoxText()
        for text in self.config.engines:
            if text == self.config.diff_engine_multi:
                combo_add_and_select(self.combo_multi, text)
            else:
                self.combo_multi.append_text(text)
        self.combo_multi.connect("changed", self.changed_cb)
        frame_multi.add(self.combo_multi)
        main_vbox.pack_start(frame_multi, True, True, 0)

        separator = Gtk.HBox(False, 5)
        main_vbox.pack_start(separator, False, True, 5)

        # cancel / ok
        confirm_hbox = Gtk.HBox(False, 0)
        main_vbox.pack_start(confirm_hbox, False, False, 0)

        cancel_button = Gtk.Button(stock=Gtk.STOCK_CANCEL)
        cancel_button.connect_object("clicked",
                                     self.cancel_event,
                                     self.window, None)
        confirm_hbox.pack_start(cancel_button, True, True, 5)

        ok_button = Gtk.Button(stock=Gtk.STOCK_OK)
        ok_button.connect_object("clicked",
                                 self.save_event,
                                 self.window, None)
        confirm_hbox.pack_start(ok_button, True, True, 5)

        self.window.show_all()

    def main(self):
        """GTK main method."""
        Gtk.main()


if __name__ == "__main__":
    prefs = NemoCompareExtensionPreferences()
    prefs.main()
