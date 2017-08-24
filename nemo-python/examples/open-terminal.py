# This example is contributed by Martin Enlund
import os
import urllib

from gi.repository import Nemo, GObject, GConf

TERMINAL_KEY = '/desktop/gnome/applications/terminal/exec'

class OpenTerminalExtension(Nemo.MenuProvider, GObject.GObject):
    def __init__(self):
        self.client = GConf.Client.get_default()

    def _open_terminal(self, file):
        filename = urllib.unquote(file.get_uri()[7:])
        terminal = self.client.get_string(TERMINAL_KEY)

        os.chdir(filename)
        os.system('%s &' % terminal)

    def menu_activate_cb(self, menu, file):
        self._open_terminal(file)

    def menu_background_activate_cb(self, menu, file):
        self._open_terminal(file)

    def get_file_items(self, window, files):
        if len(files) != 1:
            return

        file = files[0]
        if not file.is_directory() or file.get_uri_scheme() != 'file':
            return

        item = Nemo.MenuItem(name='NemoPython::openterminal_file_item',
                             label='Open Terminal' ,
                             tip='Open Terminal In %s' % file.get_name())
        item.connect('activate', self.menu_activate_cb, file)
        return item,

    def get_background_items(self, window, file):
        item = Nemo.MenuItem(name='NemoPython::openterminal_item',
                             label='Open Terminal Here',
                             tip='Open Terminal In This Directory')
        item.connect('activate', self.menu_background_activate_cb, file)
        return item,
