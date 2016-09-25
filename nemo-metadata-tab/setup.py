#!/usr/bin/python3

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-metadata-tab',
        [
            'src/nemo-metadata-tab.py'
        ]
    ),
    (
        '/usr/share/nemo-metadata-tab/exifread',
        [
            'src/exifread/__init__.py',
            'src/exifread/classes.py',
            'src/exifread/exif_log.py',
            'src/exifread/utils.py'
        ]
    ),
    (
        '/usr/share/nemo-metadata-tab/exifread/tags',
        [
            'src/exifread/tags/__init__.py',
            'src/exifread/tags/exif.py'
        ]
    ),
    (
        '/usr/share/nemo-metadata-tab/exifread/tags/makernote',
        [
            'src/exifread/tags/makernote/__init__.py',
            'src/exifread/tags/makernote/apple.py',
            'src/exifread/tags/makernote/canon.py',
            'src/exifread/tags/makernote/casio.py',
            'src/exifread/tags/makernote/fujifilm.py',
            'src/exifread/tags/makernote/nikon.py',
            'src/exifread/tags/makernote/olympus.py'
        ]
    )
]

DistUtilsExtra.auto.setup(
    name         = "nemo-metadata-tab",
    version      = "3.2.0",
    description  = "Nemo extension, adding a property page for media files",
    author       = "Eduard Dopler",
    author_email = "kontakt@eduard-dopler.de",
    url          = "https://github.com/EduardDopler/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
