import urllib

import gconf

from gi.repository import Nautilus, GObject

SUPPORTED_FORMATS = 'image/jpeg', 'image/png'
BACKGROUND_KEY = '/desktop/gnome/background/picture_filename'

class BackgroundImageExtension(GObject.GObject, Nautilus.MenuProvider):
    def __init__(self):
        self.gconf = gconf.client_get_default()
    
    def menu_activate_cb(self, menu, file):
        if file.is_gone():
            return
        
        # Strip leading file://
        filename = urllib.unquote(file.get_uri()[7:])
        self.gconf.set_string(BACKGROUND_KEY, filename)
        
    def get_file_items(self, window, files):
        if len(files) != 1:
            return

        file = files[0]

        # We're only going to put ourselves on images context menus
        if not file.get_mime_type() in SUPPORTED_FORMATS:
            return

        # Gnome can only handle file:
        # In the future we might want to copy the file locally
        if file.get_uri_scheme() != 'file':
            return

        item = Nautilus.MenuItem(name='Nautilus::set_background_image',
                                 label='Use as background image',
                                 tip='Set the current image as a background image')
        item.connect('activate', self.menu_activate_cb, file)
        return item,
