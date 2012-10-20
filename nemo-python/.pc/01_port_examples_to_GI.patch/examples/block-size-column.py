import os
import urllib

import gobject
from gi.repository import Nautilus

class ColumnExtension(gobject.GObject, Nautilus.ColumnProvider, Nautilus.InfoProvider):
    def __init__(self):
        pass
    
    def get_columns(self):
        return Nautilus.Column(name="NautilusPython::block_size_column",
                               attribute="block_size",
                               label="Block size",
                               description="Get the block size"),

    def update_file_info(self, file):
        if file.get_uri_scheme() != 'file':
            return
        
        filename = urllib.unquote(file.get_uri()[7:])
        
        file.add_string_attribute('block_size', str(os.stat(filename).st_blksize))
