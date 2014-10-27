# -*- coding: utf-8 -*-
# Folder Color 0.0.11
# Copyright (C) 2012-2014 Marcos Alvarez Costales https://launchpad.net/~costales
#
# Folder Color is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
# Folder Color is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Folder Color; if not, see http://www.gnu.org/licenses
# for more information.

import os, urllib, gettext, locale, collections
from gi.repository import Nemo, GObject, Gio, GLib, Gtk, Gdk, GdkPixbuf, cairo
_ = gettext.gettext
P_ = gettext.ngettext

import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

KNOWN_COLORS = {'Mint-X': 'Green',
                'Mint-X-Dark': 'Green',
                'Rave-X-CX': 'Beige',
                'Faience': 'Beige',
                'gnome': 'Beige',
                'Matrinileare': 'Beige',
                'menta': 'Green',
                'mate': 'Beige',
                'oxygen': 'Blue'
                }

COLORS = [ 
            'Sand',
            'Beige',
            'Yellow',
            'Orange',
            'Brown',
            'Red',
            'Purple', 
            'Pink', 
            'Blue',            
            'Cyan',
            'Aqua',
            'Teal',
            'Green',
            'White',
            'Grey',            
            'Black'
           ]

css_colors = """
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

provider = Gtk.CssProvider()
provider.load_from_data(css_colors)
screen = Gdk.Screen.get_default();
Gtk.StyleContext.add_provider_for_screen (screen, provider, 600) # GTK_STYLE_PROVIDER_PRIORITY_APPLICATION

class ChangeColorFolder(GObject.GObject, Nemo.MenuProvider):
    def __init__(self):
        print "Initializing folder-color-switcher extension..."
        self.SEPARATOR = u'\u2015' * 4
        self.DEFAULT_FOLDERS = self.get_default_folders()   
        self.settings = Gio.Settings.new("org.cinnamon.desktop.interface")
        self.get_theme()
        self.settings.connect("changed::icon-theme", self.on_theme_changed)
        pass
    
    def get_theme(self):
        self.theme =  self.settings.get_string("icon-theme")
        self.color = None
        self.base_theme = True
        self.base_color = None
        for color in COLORS:
            if self.theme.endswith("-%s" % color):
                self.theme = self.theme[:-len("-%s" % color)]
                self.color = color
                self.base_theme = False
        if not self.base_theme and KNOWN_COLORS.has_key(self.theme):
            self.base_color = KNOWN_COLORS[self.theme]

    def on_theme_changed(self, settings, key):
        self.get_theme()

    def get_default_folders(self):
        folders = {}
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_DESKTOP)]      = 'user-desktop.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_DOCUMENTS)]    = 'folder-documents.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_DOWNLOAD)]     = 'folder-download.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_MUSIC)]        = 'folder-music.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_PICTURES)]     = 'folder-pictures.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_PUBLIC_SHARE)] = 'folder-publicshare.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_TEMPLATES)]    = 'folder-templates.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_VIDEOS)]       = 'folder-video.svg'
        return folders
    
    def get_icon_path(self, folder, color):
        try:
            icon_name = self.DEFAULT_FOLDERS[folder]
        except:
            icon_name = 'folder.svg'

        if not self.base_theme and self.base_color is not None and self.base_color == color:
            path = os.path.join("/usr/share/icons/%s/places/48/%s" % (self.theme, icon_name))
        else:
            path = os.path.join("/usr/share/icons/%s-%s/places/48/%s" % (self.theme, color, icon_name))

        return path
    
    def menu_activate_cb(self, menu, color, folders):

        for each_folder in folders:
            
            if each_folder.is_gone():
                continue
            
            # Get object
            path = urllib.unquote(each_folder.get_uri()[7:])
            folder = Gio.File.new_for_path(path)
            info = folder.query_info('metadata::custom-icon', 0, None)
            
            # Set color
            if not color == 'restore':
                icon_file = Gio.File.new_for_path(self.get_icon_path(path, color))
                icon_uri = icon_file.get_uri()
                info.set_attribute_string('metadata::custom-icon', icon_uri)
            # Restore = Unset icon
            else:
                info.set_attribute('metadata::custom-icon', Gio.FileAttributeType.INVALID, 0) 
            
            # Write changes
            folder.set_attributes_from_info(info, 0, None)

            # Touch the directory to make Nemo re-render its icons
            os.system("touch \"%s\"" % path)
    
    def get_background_items(self, window, current_folder):
        return

    # Nemo invoke this function in its startup > Then, create menu entry
    def get_file_items(self, window, items_selected):

        locale.setlocale(locale.LC_ALL, '')
        gettext.bindtextdomain('folder-color-switcher')
        gettext.textdomain('folder-color-switcher')

        self.COLORS = collections.OrderedDict ([
            ('Sand', _('Sand')),
            ('Beige', _('Beige')),
            ('Yellow', _('Yellow')),
            ('Orange', _('Orange')),
            ('Brown', _('Brown')),
            ('Red', _('Red')),
            ('Purple', _('Purple')),
            ('Pink', _('Pink')), 
            ('Blue', _('Blue')),            
            ('Cyan', _('Cyan')),
            ('Aqua', _('Aqua')),
            ('Teal', _('Teal')),
            ('Green', _('Green')),
            ('White', _('White')),
            ('Grey', _('Grey')),   
            ('Black', _('Black'))
           ])

        # No items selected
        if len(items_selected) == 0:
            return
        for each_item in items_selected:
            # GNOME can only handle files
            if each_item.get_uri_scheme() != 'file':
                return
            # Only folders
            if not each_item.is_directory():
                return

        found_colors = False
        to_generate = []

        for color in self.COLORS.items():
            color_code = color[0]
            color_name = color[1]
            if not self.base_theme and self.base_color is not None and self.base_color == color_code:
                path = os.path.join("/usr/share/icons/%s/places/48/folder.svg" % self.theme)
            else:
                path = os.path.join("/usr/share/icons/%s-%s/places/48/folder.svg" % (self.theme, color_code))
            if os.path.exists(path) and (self.color is None or color_code != self.color):
                found_colors = True
                to_generate.append((color_code, color_name, items_selected))

        if (found_colors):
            item = Nemo.MenuItem(name='ChangeFolderColorMenu::Top')
            item.set_widget_a(self.generate_widget(to_generate))
            item.set_widget_b(self.generate_widget(to_generate))
            return Nemo.MenuItem.new_separator('ChangeFolderColorMenu::TopSep'),   \
                   item,                                                           \
                   Nemo.MenuItem.new_separator('ChangeFolderColorMenu::BotSep')
        else:
            return

    def generate_widget(self, to_generate):
        widget = Gtk.Box.new(Gtk.Orientation.HORIZONTAL, 1)
        button = FolderColorButton("restore")
        button.connect('clicked', self.menu_activate_cb, 'restore', to_generate[0][2])
        if (len(to_generate[0][2]) > 1):
            button.set_tooltip_text (_("Restores the color of the selected folders"))
        else:
            button.set_tooltip_text (_("Restores the color of the selected folder"))

        widget.pack_start(button, False, False, 1)

        for i in range(0, len(to_generate)):
            color_code, color_name, items_selected = to_generate[i]
            button = FolderColorButton(color_code)
            button.connect('clicked', self.menu_activate_cb, color_code, items_selected)
            if (len(items_selected) > 1):
                button.set_tooltip_markup (_("Changes the color of the selected folders to %s") % color_name.lower())
            else:
                button.set_tooltip_markup (_("Changes the color of the selected folder to %s") % color_name.lower())
                
            widget.pack_start(button, False, False, 1)

        widget.show_all()

        return widget

class FolderColorButton(Nemo.SimpleButton):
    def __init__(self, color):
        super(FolderColorButton, self).__init__()

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
            image.set_from_file("/usr/share/icons/hicolor/22x22/apps/folder-color-switcher-%s.png" % color.lower())   
            self.set_image(image)               

    def on_draw(self, widget, cr):        
        width = widget.get_allocated_width ();
        height = widget.get_allocated_height ();

        grow = 0
        line_width = 2.0

        c = self.get_style_context()

        if c.get_state() == Gtk.StateFlags.PRELIGHT:
            grow = 1
            line_width = 2.5

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

    
