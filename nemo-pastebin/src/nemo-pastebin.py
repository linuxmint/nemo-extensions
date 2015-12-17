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
import os
import signal
import subprocess
import sys
import time
import webbrowser
from threading import Thread
from urllib.parse import unquote

from gi.repository import GObject, Gdk, Gtk, Nemo, Notify

sys.path.append("/usr/share/nemo-pastebin")
import pastebin_utils as pb_utils


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


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


# internal name (for Notify)
NAME = "NemoPastebin"
# extension's name and short description
NAME_DESC = "Nemo Pastebin:::" + _("Upload files to online pastebins")


class NemoPastebin(GObject.GObject,
                   Nemo.MenuProvider,
                   Nemo.NameAndDescProvider):

    """Add a context menu item for uploading a file to a pastebin."""

    def __init__(self):
        """Load config and init Notify."""
        self.config = pb_utils.NemoPastebinConfig()
        Notify.init(NAME)

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [NAME_DESC]

    def menu_activate_cb(self, menu, files):
        """Callback for click on MenuItem. Create a PastebinThread here."""
        file_object = files[0]
        if file_object.is_gone():
            return
        filepath = unquote(file_object.get_uri()[7:])

        thread = PastebinThread(self.config, filepath)
        thread.start()
        while (thread.isAlive()):
            time.sleep(0.1)
            while Gtk.events_pending():
                Gtk.main_iteration()

    def get_file_items(self, window, files):
        """Create a MenuItem for files."""
        if len(files) != 1:
            return
        file_object = files[0]

        if ((file_object.get_uri_scheme() != "file" or
             file_object.is_directory())):
            return

        pastebin_service = self.config[pb_utils.KEY_SERVICE]
        if pastebin_service:
            menu_label = _("Upload to %s" % pastebin_service)
        else:
            menu_label = _("Upload to a pastebin service")

        item = Nemo.MenuItem(name="NemoPastebin::Upload",
                             label=menu_label,
                             tip=_("Upload this file to an online pastebin service."))
        item.connect("activate", self.menu_activate_cb, files)

        return [item]

    def get_background_items(self, window, item):
        """Return empty list. Do not react on background clicks."""
        return []


class PastebinThread(Thread):

    """A Thread for the actual pastebinit call."""

    def __init__(self, config, filepath):
        """Init the thread."""
        self.config = config
        self.filepath = filepath
        self.service = self.config[pb_utils.KEY_SERVICE]
        self.notification = None
        Thread.__init__(self)

    def run(self):
        """Call pastebinit and show notifications according to the status."""
        filename = os.path.basename(self.filepath)

        # create a notification
        # indicating that the upload has been initiated
        notify_title = _("Pastebin")
        notify_body = _("Uploading {filename}\nto {service}…").format(filename=filename,
                                                                      service=self.service)
        icon_name = "up"
        self.notification = Notify.Notification.new(notify_title, notify_body, icon_name)
        self.notification.show()

        # add our config file settings to the pastebinit call, if present
        cmd = ["pastebinit", "-i", self.filepath]
        config_keys = {
            pb_utils.KEY_SERVICE: "-b",
            pb_utils.KEY_AUTHOR: "-a",
            pb_utils.KEY_TITLE: "-t",
            pb_utils.KEY_USERNAME: "-u",
            pb_utils.KEY_PASSWORD: "-p",
            pb_utils.KEY_JABBERID: "-j",
            pb_utils.KEY_PERMATAG: "-m"
        }
        for key, abbr in config_keys.items():
            value = self.config[key]
            if value:
                cmd.extend([abbr, value])

        try:
            pastebinit_output = subprocess.check_output(args=cmd,
                                                        stderr=subprocess.STDOUT,
                                                        timeout=10)
            pastebinit_output = pastebinit_output.decode().strip()
        except subprocess.SubprocessError as e:
            self.print_error(self.notification, e.output.decode())
            return

        # maybe pastebinit returned an error message without knowing it
        # a valid answer should contain an URL
        if not pastebinit_output.startswith("http"):
            self.print_error(self.notification, pastebinit_output)
            return

        # show success notification
        # with actions for clipboard and browser
        notify_title = _("Pastebin uploaded!")
        icon_name = "emblem-default"
        self.notification.update(notify_title, pastebinit_output, icon_name)
        self.notification.add_action(action="copy_url",
                                     label=_("Copy URL"),
                                     callback=self.notification_cb,
                                     user_data=pastebinit_output)
        self.notification.add_action(action="open_browser",
                                     label=_("Open in browser"),
                                     callback=self.notification_cb,
                                     user_data=pastebinit_output)
        self.notification.show()

        # do actions according to the config file
        if self.config[pb_utils.KEY_COPYURL].title() == "True":
            copy_to_clipboard(pastebinit_output)
        if self.config[pb_utils.KEY_OPENBROWSER].title() == "True":
            webbrowser.open(pastebinit_output)

    def notification_cb(self, notification, action, user_data):
        """Callback for the notification buttons. Do the action.

        action can be either "copy_url" or "open_browser".
        """
        if action == "copy_url":
            copy_to_clipboard(user_data)
        elif action == "open_browser":
            webbrowser.open(user_data)

    def print_error(self, notification, message):
        """Print message in the present notification instance.

        Reload config after that (maybe that was the cause?).
        """
        notify_title = _("Pastebin Error")
        icon_name = "error"
        notification.update(notify_title, message, icon_name)
        notification.show()
        # reload the config
        # there might be new settings for the next time
        self.config.load()


def copy_to_clipboard(text):
    """Copy text to the system's clipboard."""
    clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
    clipboard.set_text(text, -1)
    clipboard.set_can_store()
    clipboard.store()
