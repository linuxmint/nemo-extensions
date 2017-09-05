from gi.repository import Nemo, Gtk, GObject

class LocationProviderExample(GObject.GObject, Nemo.LocationWidgetProvider):
    def __init__(self):
        pass

    def get_widget(self, uri, window):
        entry = Gtk.Entry()
        entry.set_text(uri)
        entry.show()
        return entry
