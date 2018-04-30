#!/usr/bin/python

# change log:
# geb666: original bsc.py, based on work by Giacomo Bordiga
# jmdsdf: version 2 adds extra ID3 and EXIF tag support
# jmdsdf: added better error handling for ID3 tags, added mp3 length support, distinguished
#         between exif image size and true image size
# SabreWolfy: set consistent hh:mm:ss format, fixed bug with no ID3 information
#             throwing an unhandled exception
# jmdsdf: fixed closing file handles with mpinfo (thanks gueba)
# jmdsdf: fixed closing file handles when there's an exception (thanks Pitxyoki)
# jmdsdf: added video parsing (work based on enbeto, thanks!)
# jmdsdf: added FLAC audio parsing through kaa.metadata (thanks for the idea l-x-l)
# jmdsdf: added trackno, added mkv file support (thanks ENigma885)
# jmdsdf: added date/album for flac/video (thanks eldon.t)
# jmdsdf: added wav file support thru pyexiv2
# jmdsdf: added sample rate file support thru mutagen and kaa (thanks for the idea N'ko)
# jmdsdf: fix with tracknumber for FLAC, thanks l-x-l
# draxus: support for pdf files
# arun (engineerarun@gmail.com): made changes to work with naulitus 3.x
# Andrew@webupd8.org: get EXIF support to work with Nemo 3
# Julien Blanc: fix bug caused by missing Exif.Image.Software key
# mtwebster: convert for use as a nemo extension
import os
import urllib
#import nemo
from gi.repository import Nemo, GObject, Gtk, GdkPixbuf
# for id3 support
from mutagen.easyid3 import EasyID3
from mutagen.mp3 import MPEGInfo
# for exif support
import pyexiv2
# for reading videos. for future improvement, this can also read mp3!
import kaa.metadata
# for reading image dimensions
import PIL.Image
# for reading pdf
try:
    from pyPdf import PdfFileReader
except:
    pass

import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

import gettext
gettext.bindtextdomain("nemo-extensions")
gettext.textdomain("nemo-extensions")
_ = gettext.gettext

class FileExtensionInfo():
    def __init__(self):
        self.title = None
        self.album = None
        self.artist = None
        self.tracknumber = None
        self.genre = None
        self.date = None
        self.bitrate = None
        self.samplerate = None
        self.length = None
        self.exif_datetime_original = None
        self.exif_software = None
        self.exif_flash = None
        self.exif_pixeldimensions = None
        self.exif_rating = None
        self.pixeldimensions = None

class ColumnExtension(GObject.GObject, Nemo.ColumnProvider, Nemo.InfoProvider, Nemo.NameAndDescProvider):
    def __init__(self):
        self.ids_by_handle = {}

    def get_columns(self):
        return (
            Nemo.Column(name="NemoPython::title_column",attribute="title",label=_("Title"),description=""),
            Nemo.Column(name="NemoPython::album_column",attribute="album",label=_("Album"),description=""),
            Nemo.Column(name="NemoPython::artist_column",attribute="artist",label=_("Artist"),description=""),
            Nemo.Column(name="NemoPython::tracknumber_column",attribute="tracknumber",label=_("Track"),description=""),
            Nemo.Column(name="NemoPython::genre_column",attribute="genre",label=_("Genre"),description=""),
            Nemo.Column(name="NemoPython::date_column",attribute="date",label=_("Date"),description=""),
            Nemo.Column(name="NemoPython::bitrate_column",attribute="bitrate",label=_("Bitrate"),description=""),
            Nemo.Column(name="NemoPython::samplerate_column",attribute="samplerate",label=_("Sample Rate"),description=""),
            Nemo.Column(name="NemoPython::length_column",attribute="length",label=_("Length"),description=""),
            Nemo.Column(name="NemoPython::exif_datetime_original_column",attribute="exif_datetime_original",label=_("EXIF Date"),description=""),
            Nemo.Column(name="NemoPython::exif_software_column",attribute="exif_software",label=_("EXIF Software"),description=""),
            Nemo.Column(name="NemoPython::exif_flash_column",attribute="exif_flash",label=_("EXIF Flash"),description=""),
            Nemo.Column(name="NemoPython::exif_pixeldimensions_column",attribute="exif_pixeldimensions",label=_("EXIF Image Size"),description=""),
            Nemo.Column(name="NemoPython::exif_rating",attribute="exif_rating",label=_("EXIF Rating"),description=""),
            Nemo.Column(name="NemoPython::pixeldimensions_column",attribute="pixeldimensions",label=_("Image Size"),description=""),
        )

    def set_file_attributes(self, file, info):
        for attribute in ("title", "album", "artist", "tracknumber", \
                          "genre", "date", "bitrate", "samplerate",\
                          "length", "exif_datetime_original", "exif_software", \
                          "exif_flash", "exif_pixeldimensions", "exif_rating", "pixeldimensions"):
            value = getattr(info, attribute)
            if value is None:
                file.add_string_attribute(attribute, '')
            else:
                file.add_string_attribute(attribute, value)

    def cancel_update(self, provider, handle):
        if handle in self.ids_by_handle.keys():
            GObject.source_remove(self.ids_by_handle[handle])
            del self.ids_by_handle[handle]

    def update_file_info_full(self, provider, handle, closure, file):
        if file.get_uri_scheme() != 'file':
            return Nemo.OperationResult.COMPLETE

        if handle in self.ids_by_handle.keys():
            GObject.source_remove(self.ids_by_handle[handle])

        self.ids_by_handle[handle] = GObject.idle_add(self.update_cb, provider, handle, closure, file)

        return Nemo.OperationResult.IN_PROGRESS

    def update_cb(self, provider, handle, closure, file):
        self.do_update_file_info(file)

        if handle in self.ids_by_handle.keys():
            del self.ids_by_handle[handle]

        Nemo.info_provider_update_complete_invoke(closure, provider, handle, Nemo.OperationResult.COMPLETE)

        return False

    def do_update_file_info(self, file):
        info = FileExtensionInfo()

        # strip file:// to get absolute path
        filename = urllib.unquote(file.get_uri()[7:])

        # mp3 handling
        if file.is_mime_type('audio/mpeg'):
            # attempt to read ID3 tag
            try:
                audio = EasyID3(filename)
                # sometimes the audio variable will not have one of these items defined, that's why
                # there is this long try / except attempt
                try: info.title = audio["title"][0]
                except: pass
                try: info.album = audio["album"][0]
                except: pass
                try: info.artist = audio["artist"][0]
                except: pass
                try: info.tracknumber = "{:0>2}".format(audio["tracknumber"][0])
                except: pass
                try: info.genre = audio["genre"][0]
                except: pass
                try: info.date = audio["date"][0]
                except: pass
            except:
                pass

            # try to read MP3 information (bitrate, length, samplerate)
            try:
                mpfile = open (filename)
                mpinfo = MPEGInfo (mpfile)
                info.bitrate = str(mpinfo.bitrate/1000) + " Kbps"
                info.samplerate = str(mpinfo.sample_rate) + " Hz"
                # [SabreWolfy] added consistent formatting of times in format hh:mm:ss
                # [SabreWolfy[ to allow for correct column sorting by length
                mp3length = "%02i:%02i:%02i" % ((int(mpinfo.length/3600)), (int(mpinfo.length/60%60)), (int(mpinfo.length%60)))
                mpfile.close()
                info.length = mp3length
            except:
                try:
                    mpfile.close()
                except: pass

        # image handling
        elif file.is_mime_type('image/jpeg') or file.is_mime_type('image/png') or file.is_mime_type('image/gif') or file.is_mime_type('image/bmp'):
            # EXIF handling routines
            try:
                metadata = pyexiv2.ImageMetadata(filename)
                metadata.read()
                try:
                    exif_datetimeoriginal = metadata['Exif.Photo.DateTimeOriginal']
                    info.exif_datetime_original = str(exif_datetimeoriginal.raw_value)
                except:
                    pass
                try:
                    exif_imagesoftware = metadata['Exif.Image.Software']
                    info.exif_software = str(exif_imagesoftware.raw_value)
                except:
                    pass
                try:
                    exif_photoflash = metadata['Exif.Photo.Flash']
                    info.exif_flash = str(exif_photoflash.raw_value)
                except:
                    pass
                try:
                    exif_rating = metadata['Xmp.xmp.Rating']
                    info.exif_rating = str(exif_rating.raw_value)
                except:
                    pass
            except:
                pass
            # try read image info directly
            try:
                im = PIL.Image.open(filename)
                info.pixeldimensions = str(im.size[0])+'x'+str(im.size[1])
            except error as e:
                print e
                pass

        # video/flac handling
        elif file.is_mime_type('video/x-msvideo') | file.is_mime_type('video/mpeg') | file.is_mime_type('video/x-ms-wmv') | file.is_mime_type('video/mp4') | file.is_mime_type('audio/x-flac') | file.is_mime_type('video/x-flv') | file.is_mime_type('video/x-matroska') | file.is_mime_type('audio/x-wav'):
            try:
                metadata=kaa.metadata.parse(filename)
                try: info.length = "%02i:%02i:%02i" % ((int(metadata.length/3600)), (int(metadata.length/60%60)), (int(metadata.length%60)))
                except: pass
                try: info.pixeldimensions = str(metadata.video[0].width) + 'x'+ str(metadata.video[0].height)
                except: pass
                try: info.bitrate = str(round(metadata.audio[0].bitrate/1000))
                except: pass
                try: info.samplerate = str(int(metadata.audio[0].samplerate))+' Hz'
                except: pass
                try: info.title = metadata.title
                except: pass
                try: info.artist = metadata.artist
                except: pass
                try: info.genre = metadata.genre
                except: pass
                try: info.tracknumber = metadata.trackno
                except: pass
                try: info.date = metadata.userdate
                except: pass
                try: info.album = metadata.album
                except: pass
            except:
                pass

        # pdf handling
        elif file.is_mime_type('application/pdf'):
            try:
                f = open(filename, "rb")
                pdf = PdfFileReader(f)
                try: info.title = pdf.getDocumentInfo().title
                except: pass
                try: info.artist = pdf.getDocumentInfo().author
                except: pass
                f.close()
            except:
                pass

        self.set_file_attributes(file, info)

        del info

    def get_name_and_desc(self):
        return [("Nemo Media Columns:::Provides additional columns for the list view")]
