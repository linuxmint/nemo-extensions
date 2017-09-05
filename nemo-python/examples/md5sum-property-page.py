import hashlib
import urllib

from gi.repository import Nemo, Gtk, GObject

class MD5SumPropertyPage(GObject.GObject, Nemo.PropertyPageProvider):
    def __init__(self):
        pass

    def get_property_pages(self, files):
        if len(files) != 1:
            return

        file = files[0]
        if file.get_uri_scheme() != 'file':
            return

        if file.is_directory():
            return

        filename = urllib.unquote(file.get_uri()[7:])

        self.property_label = Gtk.Label('MD5Sum')
        self.property_label.show()

        self.hbox = Gtk.HBox(homogeneous=False, spacing=0)
        self.hbox.show()

        label = Gtk.Label('MD5Sum:')
        label.show()
        self.hbox.pack_start(label, False, False, 0)

        self.value_label = Gtk.Label()
        self.hbox.pack_start(self.value_label, False, False, 0)

        md5sum = hashlib.md5()
        with open(filename,'rb') as f:
            for chunk in iter(lambda: f.read(8192), ''):
                md5sum.update(chunk)
        f.close()

        self.value_label.set_text(md5sum.hexdigest())
        self.value_label.show()

        return Nemo.PropertyPage(name="NemoPython::md5_sum",
                                 label=self.property_label,
                                 page=self.hbox),
