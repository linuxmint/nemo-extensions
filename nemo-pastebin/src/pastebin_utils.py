#!/usr/bin/python3
"""nemo-pastebin -- Nemo extension for pasting files to a pastebin service.

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

import os
import configparser
import signal
import subprocess

from xdg import BaseDirectory
# pyinotify is optional
# if present, it is used for tracking config file changes
try:
    import pyinotify
except ImportError:
    pyinotify = False


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


# configuration settings
CONFIG_FORMAT = 1
CONFIG_FILE_USER = os.path.join(BaseDirectory.xdg_config_home,
                                "nemo-pastebin.conf")
CONFIG_FILE_SYSTEM = "/etc/nemo-pastebin.conf"
SECTION_GENERAL = "DEFAULT"
KEY_FORMAT = "format"
SECTION_SETTINGS = "SETTINGS"
KEY_SERVICE = "service"
KEY_AUTHOR = "author"
KEY_USERNAME = "username"
KEY_PASSWORD = "password"
KEY_TITLE = "title"
KEY_PERMATAG = "permatag"
KEY_JABBERID = "jabberid"
KEY_OPENBROWSER = "open_browser"
KEY_COPYURL = "copy_url"


class NemoPastebinConfig():

    """Utils class for nemo-pastebin responsible for config loading."""

    def __init__(self, watch_config=True):
        """Trigger loading config file and assure correct format.

        If the format version of the config file is older than
        CONFIG_FORMAT, reset settings.
        If watch_config is set to True and pyinotify is present, call
        watch_config_file().
        """
        self.config = None
        config_filenames = self.load()
        # if old config format, reset settings
        config_format = self.config.getint(section=SECTION_GENERAL,
                                           option=KEY_FORMAT,
                                           fallback=1)
        if config_format < CONFIG_FORMAT:
            config_filenames = self.reset()

        if pyinotify and watch_config:
            self.watch_config_file(config_filenames[0])

    def __getitem__(self, key):
        """Return key from the config's settings sections.

        If the section is not present, return an empty string.
        """
        return (self.config[SECTION_SETTINGS][key]
                if key in self.config[SECTION_SETTINGS]
                else "")

    def __setitem__(self, key, value=None):
        """Store option value for key in the config's engines section.

        If key is in present in the config's settings section, write
        value to the key there. Otherwise create the section called key.
        """
        if key in self.config[SECTION_SETTINGS]:
            self.config[SECTION_SETTINGS][key] = value
        else:
            self.config[key] = {}

    def __del__(self):
        """On destruction, call stop_watching()."""
        self.stop_watching()

    def stop_watching(self):
        """Disable the watchdog."""
        if self.watch_manager:
            for watch in list(self.watch_manager.watches):
                self.watch_manager.rm_watch(watch)
        if self.notifier:
            self.notifier.stop()

    def load(self):
        """Load configuration from user or system-wide config file.

        If no file is found, reset settings.
        Return list of filenames read (should have length of 1).
        """
        self.config = configparser.ConfigParser()

        # prefer user settings over system-wide ones
        if os.path.isfile(CONFIG_FILE_USER):
            return self.config.read(CONFIG_FILE_USER)
        elif os.path.isfile(CONFIG_FILE_SYSTEM):
            return self.config.read(CONFIG_FILE_SYSTEM)
        # if no settings found, load default settings
        else:
            return self.reset()

    def reset(self):
        """Reset configuration to defaults and save it.

        Return the file written to.
        If no config file was opened, it will be.
        """
        if not self.config:
            self.load()

        self.config.clear()
        # write format version of this config file
        self.config[SECTION_GENERAL][KEY_FORMAT] = str(CONFIG_FORMAT)
        # store default settings
        self.config[SECTION_SETTINGS] = {}
        settings = self.config[SECTION_SETTINGS]
        settings[KEY_SERVICE] = "paste.ubuntu.com"
        settings[KEY_AUTHOR] = ""
        settings[KEY_USERNAME] = ""
        settings[KEY_PASSWORD] = ""
        settings[KEY_TITLE] = ""
        settings[KEY_PERMATAG] = ""
        settings[KEY_JABBERID] = ""
        settings[KEY_OPENBROWSER] = "True"
        settings[KEY_COPYURL] = "True"

        return self.save()

    def save(self):
        """Save current configuration in the per-user configuration file.

        Return the file written to.
        If no configuration is opened, open it. No need for saving then
        though.
        """
        if not self.config:
            return self.load()

        with open(CONFIG_FILE_USER, "w") as f:
            self.config.write(f)
            return CONFIG_FILE_USER

    def watch_config_file(self, config_filepath):
        """Run a watchdog through pyinotify for the config file."""
        self.watch_manager = pyinotify.WatchManager()
        watch_mask = (pyinotify.IN_DELETE |
                      pyinotify.IN_MODIFY |
                      pyinotify.IN_CREATE)

        class EventHandler(pyinotify.ProcessEvent):

            def __init__(self, update_method):
                self.update_method = update_method
                super().__init__()

            def process_IN_DELETE(self, event):
                self.update_method()

            def process_IN_MODIFY(self, event):
                self.update_method()

            def process_IN_CREATE(self, event):
                self.update_method()

        self.notifier = pyinotify.ThreadedNotifier(self.watch_manager,
                                                   EventHandler(self.load))
        self.notifier.start()
        self.wdd = self.watch_manager.add_watch(config_filepath, watch_mask)


def get_pastebin_services():
    """Return list of compatible pastebin services.

    The list is retrieved from pastebinit.
    """
    try:
        services = subprocess.check_output(["pastebinit", "-l"],
                                           stderr=subprocess.DEVNULL)
        services = services.decode().split("\n")
    except (subprocess.CalledProcessError, UnicodeError):
        return

    return [line.lstrip("- ")
            for line in services
            if line.lstrip().startswith("-")]
