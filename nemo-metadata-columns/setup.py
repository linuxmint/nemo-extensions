#!/usr/bin/python3

import DistUtilsExtra.auto

data = [
    (
        '/usr/share/nemo-metadata-columns',
        [
            'src/nemo-metadata-columns.py'
        ]
    ),
    (
        '/usr/share/nemo-metadata-columns/exifread',
        [
            'src/exifread/__init__.py',
            'src/exifread/classes.py',
            'src/exifread/exif_log.py',
            'src/exifread/utils.py'
        ]
    ),
    (
        '/usr/share/nemo-metadata-columns/exifread/tags',
        [
            'src/exifread/tags/__init__.py',
            'src/exifread/tags/exif.py'
        ]
    ),
    (
        '/usr/share/nemo-metadata-columns/exifread/tags/makernote',
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
    name         = "nemo-metadata-columns",
    version      = "3.2.0",
    description  = "Nemo extension, adding columns for media metadata",
    author       = "Eduard Dopler",
    author_email = "kontakt@eduard-dopler.de",
    url          = "https://github.com/EduardDopler/nemo-extensions",
    license      = "GPL3",
    data_files   = data
)
