from gi.repository import Nemo, GObject, Gio

SUPPORTED_FORMATS = 'image/jpeg', 'image/png'
BACKGROUND_SCHEMA = 'org.gnome.desktop.background'
BACKGROUND_KEY = 'picture-uri'

class BackgroundImageExtension(GObject.GObject, Nemo.MenuProvider):
    def __init__(self):
        self.bgsettings = Gio.Settings.new(BACKGROUND_SCHEMA)
    
    def menu_activate_cb(self, menu, file):
        if file.is_gone():
            return
        
        self.bgsettings[BACKGROUND_KEY] = file.get_uri()
        
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

        item = Nemo.MenuItem(name='Nemo::set_background_image',
                                 label='Use as background image',
                                 tip='Set the current image as a background image')
        item.connect('activate', self.menu_activate_cb, file)
        return item,
