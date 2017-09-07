#!/usr/bin/python
# -*- coding: utf-8 -*-
# nemo-pastebin - Nemo extension to paste a file to a pastebin service
# Written by:
#    Alessio Treglia <quadrispro@ubuntu.com>
# Copyright (C) 2009-2010, Alessio Treglia
#
# This package is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This package is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this package; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#

import os
import sys
import re
import gettext
from subprocess import check_output
from xdg import BaseDirectory

import gi

gi.require_version('Gtk', '3.0')
from gi.repository import Gio, Gtk

APP_NAME = "nemo-pastebin"
APP_DATA_PATHS = [os.path.join(sys.path[0], 'data')]
for dir in BaseDirectory.xdg_data_dirs:
    APP_DATA_PATHS.append(os.path.join(dir, APP_NAME))

gettext.install("nemo-extensions")

def find_data_file(path):
    target = None
    for p in APP_DATA_PATHS:
        target = os.path.join(p, path)
        if os.path.isfile(target):
            return target
    if target == None:
        raise IOError(_("Can't find %s in any known prefix!") % path)

def get_presets():
    try:
        res = check_output(["pastebinit", "-l"])
        # split the line and throw away the first slice
        res = re.split("\n-?\s?", res.strip())[1:]
    except OSError:
        sys.exit(1)
    except:
        sys.exit(-1)
    return res

UI_FILE = find_data_file('nemo-pastebin-configurator.ui')
ICON_FILE = find_data_file('nemo-pastebin.png')

class Controller(object):
    BASE_KEY = "apps.nemo-pastebin"
    def __init__(self):
        self.settings = settings = Gio.Settings.new(self.BASE_KEY)
        builder = Gtk.Builder.new()
        builder.set_translation_domain("nemo-extensions")
        builder.add_from_file(UI_FILE)

        self.dialog_main = builder.get_object('dialog_main')
        tableOptions = builder.get_object('table_options')

        # Set checkbuttons up
        openbrowser = builder.get_object('openbrowser')
        shownotification = builder.get_object('shownotification')

        openbrowser.set_active(settings.get_boolean("openbrowser"))
        settings.connect("changed::openbrowser", self.on_openbrowser_changed, openbrowser)
        openbrowser.connect('toggled', self.on_openbrowser_toggled, settings)
        shownotification.set_active(settings.get_boolean("shownotification"))
        settings.connect("changed::shownotification", self.on_shownotification_changed, shownotification)
        shownotification.connect('toggled', self.on_shownotification_toggled, settings)

        # Setup entries
        self.author = author = builder.get_object('entry_author')
        self.username = username = builder.get_object('entry_username')
        self.password = password = builder.get_object('entry_password')

        author.set_text(settings.get_string("author"))
        username.set_text(settings.get_string("username"))
        password.set_text(settings.get_string("password"))

        # Set combobox up
        self.presets = presets = get_presets()
        pastebin = builder.get_object("combobox_pastebin")
        pastebin.set_entry_text_column(0)
        curr_selected = settings.get_string("pastebin")
        pastebin.insert_text(0, curr_selected)
        #presets.remove(curr_selected)
        for i in presets:
            if i != curr_selected:
                pastebin.append_text(i)
        pastebin.set_active(0)
        settings.connect("changed::pastebin", self.on_pastebin_changed, pastebin)
        pastebin.connect("changed", self.on_pastebin_combo_changed, settings)

        builder.connect_signals(self)
        self.dialog_main.connect('delete_event', self.quit)
        self.dialog_main.show_all()

    def on_openbrowser_changed(self, settings, key, check_button):
        check_button.set_active(settings.get_boolean("openbrowser"))
    def on_shownotification_changed(self, settings, key, check_button):
        check_button.set_active(settings.get_boolean("shownotification"))

    def on_openbrowser_toggled(self, button, settings):
        settings.set_boolean("openbrowser", button.get_active())
    def on_shownotification_toggled(self, button, settings):
        settings.set_boolean("shownotification", button.get_active())

    def on_pastebin_changed(self, settings, key, combo_box):
        pass
        #combo_box.set_active(self.presets.index(settings.get_string("pastebin")))
    def on_pastebin_combo_changed(self, combo_box, settings):
        print "Selected ", combo_box.get_active_text()
        settings.set_string("pastebin", combo_box.get_active_text())

    def _save_remaining_settings(self):
        self.settings.set_string("author", self.author.get_text())
        self.settings.set_string("username", self.username.get_text())
        self.settings.set_string("password", self.password.get_text())

    def showErrorMessage(self, message):
        md = Gtk.MessageDialog.new(parent=self.dialog_main,
                                   message_format=message,
                                   buttons=Gtk.ButtonsType.CLOSE,
                                   type = Gtk.MessageType.ERROR,
                                   flags = Gtk.DialogFlags.MODAL | Gtk.DialogFlags.DESTROY_WITH_PARENT)
        md.run()
        md.destroy()

    def quit(self, *args):
        Gtk.main_quit()

    def on_button_close_clicked(self, widget, data=None):
        self._save_remaining_settings()
        self.dialog_main.destroy()
        self.quit()

def main():
    c = Controller()
    Gtk.main()

if __name__ == "__main__":
    main()
