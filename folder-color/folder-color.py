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

import os, urllib, gettext, locale
from gi.repository import Nemo, GObject, Gio, GLib
_ = gettext.gettext

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

class ChangeColorFolder(GObject.GObject, Nemo.MenuProvider):
    def __init__(self):                
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
        for color in ['Aqua', 'Beige', 'Black', 'Blue', 'Brown', 'Cyan', 'Green', 'Grey', 'Orange', 'Pink', 'Purple', 'Red', 'Teal', 'White', 'Yellow']:
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
    
    # Nemo invoke this function in its startup > Then, create menu entry
    def get_file_items(self, window, items_selected):
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

        locale.setlocale(locale.LC_ALL, '')
        gettext.bindtextdomain('folder-color')
        gettext.textdomain('folder-color')
        
        # Main menu [1 or +1 folder(s)]        
        top_menuitem = Nemo.MenuItem(name='ChangeFolderColorMenu::Top', label=_("Change color"), tip='', icon='')        
        submenu = Nemo.Menu()
        top_menuitem.set_submenu(submenu)
        
        # Colors submenu
        self.COLORS = { 'Aqua':       _("Aqua"),
                        'Beige':      _("Beige"),
                        'Black':      _("Black"),
                        'Blue':       _("Blue"),                        
                        'Brown':      _("Brown"),
                        'Cyan':       _("Cyan"),
                        'Green':      _("Green"),
                        'Grey':       _("Grey"),
                        'Orange':     _("Orange"),
                        'Pink':       _("Pink"),
                        'Purple':     _("Purple"),
                        'Red':        _("Red"),
                        'Teal':       _("Teal"),
                        'White':      _("White"),
                        'Yellow':     _("Yellow")}

        sorted_colors = sorted(self.COLORS.items(), key=lambda x:x[1])

        found_colors = False        
        for color in sorted_colors:
            if not self.base_theme and self.base_color is not None and self.base_color == color[0]:
                path = os.path.join("/usr/share/icons/%s/places/48/folder.svg" % self.theme)
            else:
                path = os.path.join("/usr/share/icons/%s-%s/places/48/folder.svg" % (self.theme, color[0]))
            if os.path.exists(path) and (self.color is None or color[0] != self.color):
                found_colors = True
                name = ''.join(['ChangeFolderColorMenu::"', color[0], '"'])
                label = color[1]
                item = Nemo.MenuItem(name=name, label=label, icon='folder-color-%s' % color[0].lower())
                item.connect('activate', self.menu_activate_cb, color[0], items_selected)
                submenu.append_item(item)            
        
        if (found_colors):        
            # Separator
            item_sep = Nemo.MenuItem(name='ChangeFolderColorMenu::Sep1', label=self.SEPARATOR, sensitive=False)
            submenu.append_item(item_sep)
            
            # Restore
            item_restore = Nemo.MenuItem(name='ChangeFolderColorMenu::Restore', label=_("Default"))
            item_restore.connect('activate', self.menu_activate_cb, 'restore', items_selected)
            submenu.append_item(item_restore)
            
            return top_menuitem,

        else:
            return
            