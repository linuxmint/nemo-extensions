#!/usr/bin/python3

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
import stopit

from urllib import parse
import gi
gi.require_version('GExiv2', '0.10')
from gi.repository import Nemo, GObject, Gtk, GdkPixbuf, GExiv2, GLib, Gio
# for id3 support
from mutagen.easyid3 import EasyID3
from mutagen.mp3 import MP3
# for reading videos. for future improvement, this can also read mp3!
from pymediainfo import MediaInfo
# for reading image dimensions
import PIL.Image
# for reading pdf
from PyPDF2 import PdfFileReader

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
        self.pages = None
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

        self.settings = Gio.Settings(schema_id="org.nemo.extensions.nemo-media-columns")
        self.load_settings(self.settings)
        self.settings.connect("changed", self.load_settings)

    def load_settings(self, settings, pspec=None, data=None):
        use_timeout = self.settings.get_boolean("use-timeout")

        # I don't think we should ever allow it to run forever, regardless
        # of preference.
        self.timeout = self.settings.get_double("timeout") if use_timeout else 30.0

        print("nemo-media-columns: using a timeout of %.2f second(s) for file processing" % self.timeout)

    def get_columns(self):
        return (
            Nemo.Column(name="NemoPython::title_column",attribute="title",label=_("Title"),description=""),
            Nemo.Column(name="NemoPython::album_column",attribute="album",label=_("Album"),description=""),
            Nemo.Column(name="NemoPython::artist_column",attribute="artist",label=_("Artist"),description=""),
            Nemo.Column(name="NemoPython::tracknumber_column",attribute="tracknumber",label=_("Track"),description=""),
            Nemo.Column(name="NemoPython::genre_column",attribute="genre",label=_("Genre"),description=""),
            Nemo.Column(name="NemoPython::date_column",attribute="date",label=_("Date"),description=""),
            Nemo.Column(name="NemoPython::bitrate_column",attribute="bitrate",label=_("Bitrate"),description=""),
            Nemo.Column(name="NemoPython::pages_column",attribute="pages",label=_("Pages"),description=""),
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
                          "genre", "date", "bitrate", "pages", "samplerate",\
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
        if file.get_uri_scheme() not in ('file', 'recent', 'favorites'):
            return Nemo.OperationResult.COMPLETE

        if handle in self.ids_by_handle.keys():
            GObject.source_remove(self.ids_by_handle[handle])

        self.ids_by_handle[handle] = GObject.idle_add(self.update_cb, provider, handle, closure, file)

        return Nemo.OperationResult.IN_PROGRESS

    def update_cb(self, provider, handle, closure, file):
        # stopit uses threads, so don't give it anything that isn't safe

        mimetype = file.get_mime_type()
        # Recent and Favorites set the G_FILE_ATTRIBUTE_STANDARD_TARGET_URI attribute
        # to their real files' locations. Use that uri in those cases.
        uri = file.get_activation_uri()
        info = None

        if uri.startswith("file"):
            try:
                with stopit.ThreadingTimeout(self.timeout):
                    info = self.get_media_info(uri, mimetype)
            except stopit.utils.TimeoutException:
                print("nemo-media-columns failed to process '%s' within a reasonable amount of time" % (gfile.get_uri(), e))

        # TODO: we shouldn't set attributes on files that didn't match any of our mimetypes.
        # we do currently so the given columns can be set to '' - we should maybe do this in
        # nemo (https://github.com/linuxmint/nemo/blob/master/libnemo-private/nemo-file.c#L6811-L6814)
        # so that here we only set files that we support.
        #
        # get_media_info can return None if nothing matched, or there was an error, instead of always
        # adding the attributes to a file whether it's useful or not.

        if info == None:
            info = FileExtensionInfo()

        # if info:
        self.set_file_attributes(file, info)
        del info

        if handle in self.ids_by_handle.keys():
            del self.ids_by_handle[handle]

        Nemo.info_provider_update_complete_invoke(closure, provider, handle, Nemo.OperationResult.COMPLETE)

        return False
 
    def get_media_info(self, uri, mimetype):
        # strip file:// to get absolute path
        filename = parse.unquote(uri[7:])

        def file_is_one_of_these(mimetype_list):
            for t in mimetype_list:
                if Gio.content_type_is_a(mimetype, t):
                    return True

        # mp3 handling
        if file_is_one_of_these(('audio/mpeg',)):
            info = FileExtensionInfo()
            # attempt to read ID3 tag
            id3_good = True
            mp3_good = True

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
            except Exception as e:
                id3_good = False

            # try to read MP3 information (bitrate, length, samplerate)
            try:
                with open(filename, 'rb') as mpfile:
                    mpinfo = MP3(mpfile).info
                    info.bitrate = str(mpinfo.bitrate / 1000) + " Kbps"
                    info.samplerate = str(mpinfo.sample_rate) + " Hz"
                    # [SabreWolfy] added consistent formatting of times in format hh:mm:ss
                    # [SabreWolfy[ to allow for correct column sorting by length
                    info.length = "%02i:%02i:%02i" % ((int(mpinfo.length/3600)), (int(mpinfo.length/60%60)), (int(mpinfo.length%60)))
            except Exception:
                mp3_good = False

            return info # if (id3_good or mp3_good) else None
        # image handling
        elif file_is_one_of_these(('image/jpeg', 'image/png', 'image/gif', 'image/bmp', 'image/tiff')):
            info = FileExtensionInfo()
            # EXIF handling routines
            exiv_good = True
            pil_good = True
            try:
                metadata = GExiv2.Metadata(path=filename)

                try:
                    info.exif_datetime_original = str(metadata.get_date_time())
                except:
                    pass

                info.exif_software = metadata.get('Exif.Image.Software', None)
                info.exif_flash = metadata.get('Exif.Photo.Flash', None)
                info.exif_rating = metadata.get('Xmp.xmp.Rating', None)
            except GLib.Error as e:
                exif = False
                
            # try read image info directly
            try:
                im = PIL.Image.open(filename)
                info.pixeldimensions = str(im.size[0])+'x'+str(im.size[1])
            except Exception as e:
                pil_good = False

            return info # if (exiv_good or pil_good) else None
        # video/flac handling
        elif file_is_one_of_these(('video/x-msvideo', 'video/mpeg', 'video/x-ms-wmv', 'video/mp4',
                                   'audio/x-flac', 'video/x-flv', 'video/x-matroska', 'audio/x-wav')):
            info = FileExtensionInfo()
            mediainfo_good = True

            try:
                mediainfo = MediaInfo.parse(filename)

                duration = 0

                for trackobj in mediainfo.tracks:
                    track = trackobj.to_data()

                    if track["track_type"] == "Video":
                        try:
                            info.pixeldimensions = "%dx%d" % (track["width"], track["height"])
                        except:
                            pass

                        try:
                            duration = int(float(track['duration']))
                        except:
                            pass

                    if track["track_type"] == "Audio":
                        try:
                            info.samplerate = track['other_sampling_rate'][0]
                        except:
                            pass
                        try:
                            if duration == 0:
                                duration = int(track['duration'])
                        except:
                            pass

                    if track["track_type"] == "General":
                        try:
                            info.bitrate = track['other_overall_bit_rate'][0]
                        except:
                            pass
                        try:
                            if duration == 0:
                                duration = int(track['duration'])
                        except:
                            pass
                        try:
                            info.title = track['track_name']
                        except:
                            pass
                        try:
                            info.artist = track['performer']
                        except:
                            pass
                        try:
                            info.genre = track['genre']
                        except:
                            pass
                        try:
                            info.tracknumber = track['track_name_position']
                        except:
                            pass
                        try:
                            info.date = track['recorded_date']
                        except:
                            pass
                        try:
                            info.album = track['album']
                        except:
                            pass

                if duration > 0:
                    seconds = duration / 1000 # ms to s
                    info.length = "%02i:%02i:%02i" % ((seconds / 3600), (seconds / 60 % 60), (seconds % 60))
            except Exception as e:
                mediainfo_good = False

            return info #if mediainfo_good else None

        # pdf handling
        elif file_is_one_of_these(('application/pdf',)):
            info = FileExtensionInfo()
            pdf_good = True

            try:
                with open(filename, "rb") as f:
                    pdf = PdfFileReader(f)
                    try: info.title = pdf.getDocumentInfo().title
                    except: pass
                    try: info.artist = pdf.getDocumentInfo().author
                    except: pass
                    try: info.pages = str(pdf.getNumPages())
                    except: pass
            except:
                pdf_good = False

            return info # if pdf_good else None

        # return None # TODO - not a file we care about, we shouldn't add attributes to a file in this case.
        return FileExtensionInfo()

    def get_name_and_desc(self):
        return [("Nemo Media Columns:::Provides additional columns for the list view:::nemo-media-columns-prefs")]
