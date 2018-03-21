import os

# A way to get unquote working with python 2 and 3
try:
    from urllib import unquote
except ImportError:
    from urllib.parse import unquote

from gi.repository import GObject, Nemo

class ColumnExtension(GObject.GObject, Nemo.ColumnProvider, Nemo.InfoProvider):
    def __init__(self):
        pass

    def get_columns(self):
        return Nemo.Column(name="NemoPython::block_size_column",
                           attribute="block_size",
                           label="Block size",
                           description="Get the block size"),

    def update_file_info(self, file):
        if file.get_uri_scheme() != 'file':
            return

        filename = unquote(file.get_uri()[7:])

        file.add_string_attribute('block_size', str(os.stat(filename).st_blksize))
