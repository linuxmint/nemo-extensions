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

import os
import configparser
import signal
from collections import OrderedDict
from itertools import product

from xdg import BaseDirectory


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


# configuration settings
CONFIG_FORMAT = 2
CONFIG_FILE_USER = os.path.join(BaseDirectory.xdg_config_home,
                                "nemo-compare.conf")
CONFIG_FILE_SYSTEM = "/etc/nemo-compare.conf"
SECTION_GENERAL = "DEFAULT"
KEY_FORMAT = "format"
SECTION_ENGINES = "Engines"
KEY_2WAY = "2way"
KEY_3WAY = "3way"
KEY_NWAY = "nway"

# engines and their capabilites for comparison
ENGINES_KNOWN = OrderedDict(
    (
        ("meld", KEY_3WAY),
        ("diffuse", KEY_NWAY),
        ("kdiff3", KEY_3WAY),
        ("kompare", KEY_2WAY),
        ("fldiff", KEY_2WAY),
        ("tkdiff", KEY_2WAY)
    )
)
# search path for engines
ENGINE_PATHS = (
    "/usr/bin",
    "/usr/local/bin"
)


class NemoCompareConfig():

    """Utils class for nemo-compare responsible for config loading."""

    def __init__(self):
        """Trigger loading config file and assure correct format.

        If the format version of the config file is older than
        CONFIG_FORMAT, reset settings.
        """
        self.config = None
        self.load()
        self.update_engines()
        # if old config format, reset settings
        config_format = self.config.getint(section=SECTION_GENERAL,
                                           option=KEY_FORMAT,
                                           fallback=1)
        if config_format < CONFIG_FORMAT:
            self.reset()

    def __getitem__(self, key):
        """Return section or option value for key from configuration.

        If key is one of KEY_2WAY, KEY_3WAY or KEY_NWAY, assume section is
        "Engines", hence return their option values. On any other key,
        return section for that key.
        """
        if key in (KEY_2WAY, KEY_3WAY, KEY_NWAY):
            return self.config.get(section=SECTION_ENGINES,
                                   option=key,
                                   fallback=None)
        else:
            return (self.config[key]
                    if self.config.has_section(key)
                    else None)

    def __setitem__(self, key, value=None):
        """Store option value for key in the config's engines section.

        If key is not one of KEY_2WAY, KEY_3WAY or KEY_NWAY, create a new
        empty section.
        """
        if key in (KEY_2WAY, KEY_3WAY, KEY_NWAY):
            self.config[SECTION_ENGINES][key] = value
        else:
            self.config[key] = {}

    def load(self):
        """Load configuration from user or system-wide config file.

        If no file is found, reset settings.
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

    def update_engines(self):
        """Remove all engines from config which aren't installed anymore.

        Force reading the current config file before as the user may
        have changed it.
        """
        self.load()

        installed = get_installed_engines()
        config_engines = self.config[SECTION_ENGINES]
        changed = False
        for engine_type, engines in installed.items():
            if config_engines[engine_type] not in installed[engine_type]:
                config_engines[engine_type] = (engines[0]
                                               if engines
                                               else None)
                changed = True
        if changed:
            self.save()

    def reset(self):
        """Reset configuration to defaults and save it.

        All engines are set to the first installed engine with the
        appropriate capabilities. If none, the value will be empty.
        If no config file was opened, it will be.
        """
        if not self.config:
            self.load()

        self.config.clear()
        # write format version of this config file
        self.config[SECTION_GENERAL][KEY_FORMAT] = "2"
        # get installed engines, grouped by capabilities
        engines_installed = get_installed_engines()
        # store first engine found for every capability
        self.config[SECTION_ENGINES] = {}
        config_engines = self.config[SECTION_ENGINES]
        config_engines[KEY_2WAY] = engines_installed[KEY_2WAY][0]
        config_engines[KEY_3WAY] = engines_installed[KEY_3WAY][0]
        config_engines[KEY_NWAY] = engines_installed[KEY_NWAY][0]

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

    def can_compare(self, amount):
        """TODO."""
        return bool(get_engine_for_amount(self.config, amount))


def get_installed_engines():
    """Return a dict of installed engines grouped by capabilities.

    "Capabilities" means either KEY_2WAY, KEY_3WAY or KEY_NWAY.
    We search for ENGINES_KNOWN in ENGINE_PATHS, first-in-first-out,
    no caching.
    """
    installed = {
        KEY_2WAY: [],
        KEY_3WAY: [],
        KEY_NWAY: []
    }
    for path, engine in product(ENGINE_PATHS, ENGINES_KNOWN.keys()):
        filepath = os.path.join(path, engine)
        if os.path.isfile(filepath):
            engine_type = ENGINES_KNOWN[engine]
            installed[KEY_2WAY].append(engine)
            if engine_type == KEY_3WAY:
                installed[KEY_3WAY].append(engine)
            elif engine_type == KEY_NWAY:
                installed[KEY_NWAY].append(engine)
                installed[KEY_3WAY].append(engine)
    return installed


def get_engine_for_amount(config, amount):
    """TODO."""
    config_engines = config[SECTION_ENGINES]
    if ((amount == 2 and
         config_engines[KEY_2WAY])):
        return config_engines[KEY_2WAY]
    elif ((amount == 3 and
           config_engines[KEY_3WAY])):
        return config_engines[KEY_3WAY]
    elif ((amount > 3 and
           config_engines[KEY_NWAY])):
        return config_engines[KEY_NWAY]
