# -*- coding: utf-8 -*-
# Folder Color 0.0.9
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

import os, urllib, gettext, locale, webbrowser
from gi.repository import Nemo, GObject, Gio, GLib

# i18n
locale.setlocale(locale.LC_ALL, '')
gettext.bindtextdomain('folder-color')
gettext.textdomain('folder-color')
_ = gettext.gettext


class ChangeColorFolder(GObject.GObject, Nemo.MenuProvider):
    def __init__(self):
        self.COLORS = {'black':      _("Black"),
                       'brown':      _("Brown"),
                       'cyan':       _("Cyan"),
                       'blue':       _("Blue"),
                       'green':      _("Green"),
                       'grey':       _("Grey"),
                       'orange':     _("Orange"),
                       'pink':       _("Pink"),
                       'red':        _("Red"),
                       'purple':     _("Purple"),
                       'yellow':     _("Yellow") }
        
        self.SEPARATOR = u'\u2015' * 4
        self.HIDE_PATH = os.path.join(os.getenv("HOME"), '.config', 'folder-color')
        self.FOLDER_ICONS = '/usr/share/folder-color/media/'
        self.DEFAULT_FOLDERS = self.get_default_folders()
        
        pass
    
    def get_default_folders(self):
        folders = {}
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_DESKTOP)]      = 'Desktop.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_DOCUMENTS)]    = 'Documents.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_DOWNLOAD)]     = 'Downloads.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_MUSIC)]        = 'Music.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_PICTURES)]     = 'Pictures.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_PUBLIC_SHARE)] = 'Public.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_TEMPLATES)]    = 'Templates.svg'
        folders[GLib.get_user_special_dir(GLib.USER_DIRECTORY_VIDEOS)]       = 'Videos.svg'
        return folders
    
    def get_icon_path(self, folder, color):
        try:
            icon_name = self.DEFAULT_FOLDERS[folder]
        except:
            icon_name = 'Folder.svg'
        
        return os.path.join(self.FOLDER_ICONS, color, icon_name)
    
    def reload_icons(self):
        # Select all + Unselect all + Reload = Reload without selected folders
        os.system("xte \
                  'keydown Control_L' 'key A' 'keyup Control_L' \
                  'keydown Shift_L' 'keydown Control_L' 'key I' 'keyup Control_L' 'keyup Shift_L' \
                  'keydown Control_L' 'key R' 'keyup Control_L'")
    
    def menu_activate_cb(self, menu, color, folders):
        
        for each_folder in folders:
            
            if each_folder.is_gone():
                continue
            
            # Get object
            folder = Gio.File.new_for_path(urllib.unquote(each_folder.get_uri()[7:]))
            info = folder.query_info('metadata::custom-icon', 0, None)
            
            # Set color
            if not color == 'restore':
                icon_file = Gio.File.new_for_path(self.get_icon_path(each_folder.get_uri()[7:], color))
                icon_uri = icon_file.get_uri()
                info.set_attribute_string('metadata::custom-icon', icon_uri)
            # Restore = Unset icon
            else:
                info.set_attribute('metadata::custom-icon', Gio.FileAttributeType.INVALID, 0) 
            
            # Write changes
            folder.set_attributes_from_info(info, 0, None)
        
        # Reload changes
        self.reload_icons()
    
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
        
        # Main menu [1 or +1 folder(s)]
        if len(items_selected) > 1:
            top_menuitem = Nemo.MenuItem(name='ChangeFolderColorMenu::Top', label=_("Folders Color"), tip='', icon='')
        else:
            top_menuitem = Nemo.MenuItem(name='ChangeFolderColorMenu::Top', label=_("Folder Color"), tip='', icon='')
        submenu = Nemo.Menu()
        top_menuitem.set_submenu(submenu)
        
        # Colors submenu
        sorted_colors = sorted(self.COLORS.items(), key=lambda x:x[1])
        for color in sorted_colors:
            name = ''.join(['ChangeFolderColorMenu::"', color[0], '"'])
            label = color[1]
            item = Nemo.MenuItem(name=name, label=label, icon='')
            item.connect('activate', self.menu_activate_cb, color[0], items_selected)
            submenu.append_item(item)
        
        # Separator
        item_sep = Nemo.MenuItem(name='ChangeFolderColorMenu::Sep1', label=self.SEPARATOR, sensitive=False)
        submenu.append_item(item_sep)
        
        # Restore
        item_restore = Nemo.MenuItem(name='ChangeFolderColorMenu::Restore', label=_("Default"))
        item_restore.connect('activate', self.menu_activate_cb, 'restore', items_selected)
        submenu.append_item(item_restore)

        return top_menuitem,
