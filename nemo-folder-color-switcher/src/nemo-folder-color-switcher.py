#!/usr/bin/python3
"""nemo-folder-color-switcher -- Nemo extension for changing folder colors.

Copyright (C) 2012-2014 Marcos Alvarez Costales (launchpad.net/~costales)
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

import os
import gettext
import locale
import signal
import subprocess
from urllib import parse

from gi.repository import GObject, Gio, GLib, Gtk, Gdk, Nemo


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


TEXTDOMAIN = "nemo-folder-color-switcher"


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
NAME_DESC = "Nemo Folder Color Switcher:::" + \
            _("Change folder colors via context menu")

BUTTONS_ICON_PATH = "/usr/share/icons/hicolor/10x10/apps/" \
                    "nemo-folder-color-switcher-%s.png"
FOLDER_ICONS_PATH = "/usr/share/icons/{theme}/places/48/{filename}"
DEFAULT_FOLDER_FILENAME = "folder.svg"

SPECIAL_DIRS = {
    GLib.USER_DIRECTORY_DESKTOP: "user-desktop.svg",
    GLib.USER_DIRECTORY_DOCUMENTS: "folder-documents.svg",
    GLib.USER_DIRECTORY_DOWNLOAD: "folder-download.svg",
    GLib.USER_DIRECTORY_MUSIC: "folder-music.svg",
    GLib.USER_DIRECTORY_PICTURES: "folder-pictures.svg",
    GLib.USER_DIRECTORY_PUBLIC_SHARE: "folder-publicshare.svg",
    GLib.USER_DIRECTORY_TEMPLATES: "folder-templates.svg",
    GLib.USER_DIRECTORY_VIDEOS: "folder-video.svg"
}

GSETTINGS_SCHEMA = "org.cinnamon.desktop.interface"
GSETTINGS_KEY_ICON_THEME = "icon-theme"
SIGNAL_CHANGED_ICON_THEME = "changed::icon-theme"

METADATA_CUSTOM_ICON = "metadata::custom-icon"

COLORS = (
    "Sand",
    "Beige",
    "Yellow",
    "Orange",
    "Brown",
    "Red",
    "Purple",
    "Pink",
    "Blue",
    "Cyan",
    "Aqua",
    "Teal",
    "Green",
    "White",
    "Grey",
    "Black"
)
# (These are listed here for the single purpose to be found by translation
# software as in the further code there are accessed mostly indirectly.)
(
    _("Sand"),
    _("Beige"),
    _("Yellow"),
    _("Orange"),
    _("Brown"),
    _("Red"),
    _("Purple"),
    _("Pink"),
    _("Blue"),
    _("Cyan"),
    _("Aqua"),
    _("Teal"),
    _("Green"),
    _("White"),
    _("Grey"),
    _("Black")
)

CSS_BUTTONS = """
.folder-color-switcher-button {
    border-style: solid;
    border-width: 1px;
    border-radius: 1px;
    border-color: transparent;
}

.folder-color-switcher-button:hover {
    border-color: #9c9c9c;
}

.folder-color-switcher-restore {
    background-color: transparent;
}

.folder-color-switcher-restore:hover {
    background-color: rgba(255,255,255,0);
}
"""


class NemoFolderColorSwitcher(GObject.GObject,
                              Nemo.MenuProvider,
                              Nemo.NameAndDescProvider):

    """Nemo extension for changing folder colors.

    If the current theme provides color variations, let the user choose
    between them for one or multiple selected folders from the context
    menu.
    """

    def __init__(self):
        """Load current theme."""
        self._init_css()
        self.gsettings = Gio.Settings(GSETTINGS_SCHEMA)
        self.theme_base = self.get_theme()
        self.installed_colors = get_installed_colors(self.theme_base)
        self.gsettings.connect(SIGNAL_CHANGED_ICON_THEME,
                               self.on_theme_changed)

    def _init_css(self):
        """Load CSS data for the context menu widget button."""
        provider = Gtk.CssProvider()
        provider.load_from_data(CSS_BUTTONS.encode())
        screen = Gdk.Screen.get_default()
        priority = Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        Gtk.StyleContext.add_provider_for_screen(screen, provider, priority)

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [NAME_DESC]

    def get_theme(self):
        """Return current theme base name (without -color, if any).

        If the theme name from GSettings ends with a color we defined in
        COLORS, we assume the part before that is the base name. Else
        return the whole theme name.
        """
        theme_name = self.gsettings.get_string(GSETTINGS_KEY_ICON_THEME)
        theme_base, _, theme_color = theme_name.rpartition("-")
        if theme_color not in COLORS:
            theme_base = theme_name
        return theme_base

    def on_theme_changed(self, settings, key):
        """React on theme changes. Update theme_base and installed_colors."""
        self.theme_base = self.get_theme()
        self.installed_colors = get_installed_colors(self.theme_base)

    def menu_activate_cb(self, menu, color, folders):
        """Change the icons of "folders" to "color".

        If color is "restore", restore the theme's default folder icon.
        Callback method for clicks on the context menu widgets.
        """
        for each_folder in folders:
            if each_folder.is_gone():
                continue

            path = parse.unquote(each_folder.get_uri()[7:])
            folder = Gio.File.new_for_path(path)
            info = folder.query_info(METADATA_CUSTOM_ICON, 0, None)

            if color == "restore":
                # unsetting is done via the INVALID constant and value_p 0
                info.set_attribute(METADATA_CUSTOM_ICON,
                                   type=Gio.FileAttributeType.INVALID,
                                   value_p=0)
            else:
                icon_path = get_icon_path(path, self.theme_base, color)
                if not icon_path:
                    continue
                icon_uri = "file://" + parse.quote(icon_path)
                info.set_attribute_string(METADATA_CUSTOM_ICON, icon_uri)

            folder.set_attributes_from_info(info, flags=0, cancellable=None)

            # touch the file (to force Nemo to re-render its icon)
            try:
                subprocess.call(["touch", path])
            except OSError:
                pass

    def get_file_items(self, window, items):
        """Return the context menu widget.

        Called by Nemo on view changes. items are the currently selected
        items.
        """
        if not items:
            return
        # skip if current theme has no other colors
        if not self.installed_colors:
            return
        # only folders and scheme is file
        if any(item.get_uri_scheme() != "file" or
               not item.is_directory()
               for item in items):
            return

        widget_a = generate_menu_widget(self.installed_colors, items,
                                        callback=self.menu_activate_cb)
        widget_b = generate_menu_widget(self.installed_colors, items,
                                        callback=self.menu_activate_cb)

        menuItem = Nemo.MenuItem(name="NemoFolderColorSwitcher::Widget",
                                 widget_a=widget_a,
                                 widget_b=widget_b)
        return (Nemo.MenuItem.new_separator("NemoFolderColorSwitcher::TopSep"),
                menuItem,
                Nemo.MenuItem.new_separator("NemoFolderColorSwitcher::BotSep"))

    def get_background_items(self, window, current_folder):
        """Return empty list. Do not react on background clicks."""
        return []


class NemoFolderColorButton(Nemo.SimpleButton):

    """A Button for the nemo-folder-color-switcher context menu widget."""

    def __init__(self, color):
        """Create a colored button or a restore button.

        If color is "restore", create a button with an X instead of a
        colored one.
        """
        super().__init__()

        c = self.get_style_context()

        if color == "restore":
            c.add_class("folder-color-switcher-restore")
            self.da = Gtk.DrawingArea.new()
            self.da.set_size_request(12, 10)
            self.set_image(self.da)
            self.da.connect("draw", self.on_draw)
        else:
            c.add_class("folder-color-switcher-button")
            image = Gtk.Image()
            image.set_from_file(BUTTONS_ICON_PATH % color.lower())
            self.set_image(image)

    def on_draw(self, widget, cr):
        """Drawing function.

        Add a zoom effect for the restore button and a border for the
        colors.
        """
        width = widget.get_allocated_width()
        height = widget.get_allocated_height()

        if self.get_style_context().get_state() == Gtk.StateFlags.PRELIGHT:
            grow = 1
            line_width = 2.5
        else:
            grow = 0
            line_width = 2.0
        cr.save()

        cr.set_source_rgb(0, 0, 0)
        cr.set_line_width(line_width)
        cr.set_line_cap(1)

        cr.move_to(3 - grow, 2 - grow)
        cr.line_to(width - 3 + grow, height - 2 + grow)
        cr.move_to(3 - grow, height - 2 + grow)
        cr.line_to(width - 3 + grow, 2 - grow)

        cr.stroke()
        cr.restore()

        return False


def get_installed_colors(theme_base):
    """Return a list of available colors for the theme base name."""
    installed_colors = []
    for color in COLORS:
        maybe_theme = theme_base + "-" + color
        color_path = FOLDER_ICONS_PATH.format(theme=maybe_theme,
                                              filename=DEFAULT_FOLDER_FILENAME)
        if os.path.exists(color_path):
            installed_colors.append(color)
    return installed_colors


def get_icon_path(folder, theme_base, color):
    """Return the full icon path for this color, theme and folder.

    As there are special folders (like the Music or Desktop folder),
    we check here whether to return the path of one of those or the
    default folder icon if folder is not special.
    If that path does not exist, return None.
    """
    special_dirs = special_dirs_icon_filenames()
    icon_filename = special_dirs.get(folder, DEFAULT_FOLDER_FILENAME)

    colored_theme = theme_base + "-" + color
    path = FOLDER_ICONS_PATH.format(theme=colored_theme,
                                    filename=icon_filename)
    if os.path.exists(path):
        return path
    else:
        return None


def special_dirs_icon_filenames():
    """Return a dict of {special_dir_path: special_dir_icon_filename}."""
    return {GLib.get_user_special_dir(g_user_dir_id): icon_filename
            for g_user_dir_id, icon_filename in SPECIAL_DIRS.items()}


def generate_menu_widget(installed_colors, items, callback):
    """Return a horizontal box holding the NemoFolderColorButtons."""
    box = Gtk.HBox()
    button = NemoFolderColorButton("restore")
    button.connect("clicked", callback, "restore", items)
    if len(items) == 1:
        tooltip = _("Restores the color of the selected folder")
    else:
        tooltip = _("Restores the color of the selected folders")
    button.set_tooltip_text(tooltip)
    box.pack_start(button, False, False, 1)

    for color in installed_colors:
        button = NemoFolderColorButton(color)
        button.connect("clicked", callback, color, items)
        if len(items) == 1:
            tooltip = (_("Changes the color of the selected folder to %s") %
                       _(color).lower())
        else:
            tooltip = (_("Changes the color of the selected folders to %s") %
                       _(color).lower())
        button.set_tooltip_text(tooltip)
        box.pack_start(button, False, False, 1)

    box.show_all()
    return box
