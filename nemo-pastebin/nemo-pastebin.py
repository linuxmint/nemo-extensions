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
import time
import gettext
import re
import urllib
import webbrowser
from subprocess import check_output, CalledProcessError
from threading import Thread
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import GObject, Gdk, Gio, Gtk
gi.require_version('Nemo', '3.0')
from gi.repository import Nemo

try:
    gi.require_version('Notify', '0.7')
    from gi.repository import Notify
except ImportError:
    Notify = None

# Globals
APP_NAME = "nemo-pastebin"
LOGGING = 1
MAINTAINER_MODE = True

gettext.install("nemo-extensions")

class log:
    def log(self, message):
        if self.logger:
            self.logger.debug(message)
    def __init__(self):
        logger = None
        if LOGGING:
            import logging
            logger = logging.getLogger(APP_NAME)
            logger.setLevel(logging.DEBUG)
            ch = logging.StreamHandler()
            ch.setLevel(logging.DEBUG)
            logger.addHandler(ch)
        self.logger = logger

log = log().log

Notify.init("Nemo pastebin extension")

class PastebinThread(Thread):
    def __init__(self, settings, filename):
        self.settings = settings
        self.filename = filename
        Thread.__init__(self)

    def run(self):
        log ("PastebinThread started!")
        cmdline = ["pastebinit"]

        options = {}
        options['-b'] = "http://" + self.settings.get_string("pastebin")
        options['-a'] = self.settings.get_string("author")
        options['-u'] = self.settings.get_string("username")
        options['-p'] = self.settings.get_string("password")

        if not options['-a'] or options['-a'].strip() == '':
            options['-a'] = os.getenv("USERNAME")

        for opt in options:
            if options[opt] != None and options[opt].strip() != '':
                cmdline.append(opt)
                cmdline.append(options[opt])

        cmdline.append(self.filename)

        summary = ''
        message = ''
        icon = None
        helper = Gtk.Button()

        try:
            pasteurl = check_output(cmdline).strip()
        except CalledProcessError as error:
            # error.cmd and error.returncode, pasteurl has strerr
            summary = error.message
            icon = helper.render_icon(Gtk.STOCK_DIALOG_ERROR, Gtk.IconSize.DIALOG, None)
        except:
            #        if not pasteurl or not re.match("^http://.*$", pasteurl):
            summary = _("Unable to read or parse the result page.")
            message = _("It could be a server timeout or a change server side. Try later.")
            icon = helper.render_icon(Gtk.STOCK_DIALOG_ERROR, Gtk.IconSize.DIALOG, None)
        else:
            summary = 'File pasted to: '
            #URLmarkup = '<a href="%(pasteurl)s">%(pasteurl)s</a>'
            #message = URLmarkup % {'pasteurl':pasteurl}
            message = pasteurl
            icon = helper.render_icon(Gtk.STOCK_PASTE, Gtk.IconSize.DIALOG, None)

            atom = Gdk.atom_intern('CLIPBOARD', True)
            cb = Gtk.Clipboard.get(atom)
            cb.clear()
            cb.set_text(pasteurl, -1)

            # Open a browser window
            if self.settings['openbrowser']:
                webbrowser.open(pasteurl)

        # Show a bubble
        if self.settings['shownotification']:
            if Notify != None:
                n = Notify.Notification.new(summary, message, None)
                n.set_image_from_pixbuf(icon)
                n.show()
            else:
                print "libnotify is not installed"

class PastebinitExtension(GObject.GObject, Nemo.MenuProvider, Nemo.NameAndDescProvider):
    BASE_KEY = "apps.nemo-pastebin"
    def __init__(self):
        log ("Initializing nemo-pastebin extension...")
        self.settings = Gio.Settings.new(self.BASE_KEY)
        self.validate_settings()

    def validate_settings(self):
        # Make sure "pastebin" value is supported by pastebinit
        pass #TODO

    def show_error_message(self, error, description):
        n = Notify.Notification.new(error, description, None)
        helper = Gtk.Button()
        icon = helper.render_icon(Gtk.STOCK_DIALOG_ERROR, Gtk.IconSize.DIALOG)
        n.set_icon_from_pixbuf(icon)
        n.show()

    def get_file_items(self, window, files):
        if len(files)!=1:
            return
        filename = files[0]

        if filename.get_uri_scheme() != 'file' or filename.is_directory():
            return

        items = []
        pastebin = self.settings.get_string("pastebin")
        #Called when the user selects a file in Nemo.
        item = Nemo.MenuItem(name="NemoPython::pastebin_item",
                             label=_("Pastebin to %s") % pastebin,
                             tip=_("Send this file to %s") % pastebin)
        item.set_property('icon', "nemo-pastebin")
        item.connect("activate", self.menu_activate_cb, files)
        items.append(item)
        return items

    def get_background_items(self, window, item):
        return []

    def menu_activate_cb(self, menu, files):
        if len(files) != 1:
            return
        filename = files[0]

        # TODO: MIME check

        filename = urllib.unquote(filename.get_uri()[7:])

        thread = PastebinThread(self.settings, filename)
        thread.start()
        while (thread.isAlive()):
            time.sleep(0.09)
            while Gtk.events_pending():
                Gtk.main_iteration()

    def get_name_and_desc(self):
        return [("Nemo Pastebin:::Send files to a paste service via the context menu:::nemo-pastebin-configurator")]
