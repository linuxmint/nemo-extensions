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
import gtk
import locale

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
    """The main class for a preferences dialog using PyGTK."""

    combo = None
    combo_3way = None
    combo_multi = None

    def cancel_event(self, widget, event, data=None):
        """Quit the program."""
        gtk.main_quit()
        return False

    def changed_cb(self, combo):
        """Any of the comboboxes has changed, change the data accordingly."""
        model = combo.get_model()
        index = combo.get_active()
        entry = combo.get_child().get_text()

        selection = ""

        if len(entry) > 0:
            selection = entry
        elif index:
            selection = model[index][0]

        if combo is self.combo:
            self.config.diff_engine = selection
        elif combo is self.combo_3way:
            self.config.diff_engine_3way = selection
        elif combo is self.combo_multi:
            self.config.diff_engine_multi = selection

        return

    def save_event(self, widget, event, data=None):
        """Save settings and quit program."""
        self.config.save()
        gtk.main_quit()
        return False

    def __init__(self):
        """Load config and create UI."""

        self.config = utils.NemoCompareConfig()
        self.config.load()

        # find out if some new engines have been installed
        self.config.add_missing_predefined_engines()

        # initialize i18n
        locale.setlocale(locale.LC_ALL, "")
        gettext.bindtextdomain(utils.APP)
        gettext.textdomain(utils.APP)
        _ = gettext.gettext

        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        #self.window.set_position(gtk.WIN_POS_CENTER)
        icon = self.window.render_icon(gtk.STOCK_PREFERENCES,
                                       gtk.ICON_SIZE_DIALOG)
        self.window.set_icon(icon)
        self.window.set_resizable(False)
        self.window.set_title(_("Nemo Compare Extension Preferences"))
        self.window.connect("delete_event", self.cancel_event)
        self.window.set_border_width(15)

        main_vbox = gtk.VBox(False, 0)
        self.window.add(main_vbox)

        # normal diff
        frame = gtk.Frame(_("Normal Diff"))
        self.combo = gtk.combo_box_entry_new_text()
        for text in self.config.engines:
            if text == self.config.diff_engine:
                combo_add_and_select(self.combo, text)
            else:
                self.combo.append_text(text)
        self.combo.connect("changed", self.changed_cb)
        frame.add(self.combo)
        main_vbox.pack_start(frame, True, True, 0)

        # 3-way diff
        frame_3way = gtk.Frame(_("Three-Way Diff"))
        self.combo_3way = gtk.combo_box_entry_new_text()
        for text in self.config.engines:
            if text == self.config.diff_engine_3way:
                combo_add_and_select(self.combo_3way, text)
            else:
                self.combo_3way.append_text(text)
        self.combo_3way.connect("changed", self.changed_cb)
        frame_3way.add(self.combo_3way)
        main_vbox.pack_start(frame_3way, True, True, 0)

        # n-way diff
        frame_multi = gtk.Frame(_("N-Way Diff"))
        self.combo_multi = gtk.combo_box_entry_new_text()
        for text in self.config.engines:
            if text == self.config.diff_engine_multi:
                combo_add_and_select(self.combo_multi, text)
            else:
                self.combo_multi.append_text(text)
        self.combo_multi.connect("changed", self.changed_cb)
        frame_multi.add(self.combo_multi)
        main_vbox.pack_start(frame_multi, True, True, 0)

        separator = gtk.HBox(False, 5)
        main_vbox.pack_start(separator, False, True, 5)

        # cancel / ok
        confirm_hbox = gtk.HBox(False, 0)
        main_vbox.pack_start(confirm_hbox, False, False, 0)

        cancel_button = gtk.Button(stock=gtk.STOCK_CANCEL)
        cancel_button.connect_object("clicked",
                                     self.cancel_event,
                                     self.window, None)
        confirm_hbox.pack_start(cancel_button, True, True, 5)

        ok_button = gtk.Button(stock=gtk.STOCK_OK)
        ok_button.connect_object("clicked",
                                 self.save_event,
                                 self.window, None)
        confirm_hbox.pack_start(ok_button, True, True, 5)

        self.window.show_all()

    def main(self):
        """GTK main method."""
        gtk.main()


if __name__ == "__main__":
    prefs = NemoCompareExtensionPreferences()
    prefs.main()
