#!/usr/bin/python2

# change log:
# RavetcoFX: Forked from nemo-media-columns and nemo-emblems

import urllib
import locale, gettext, os
import mutagen

from gi.repository import GObject, Gio, Gtk, Nemo
# for id3 support
from mutagen.easyid3 import EasyID3
from mutagen.mp3 import MPEGInfo
from mutagen.flac import FLAC, StreamInfo

class AudioPropertyPage(GObject.GObject, Nemo.PropertyPageProvider, Nemo.NameAndDescProvider):

    def get_property_pages(self, files):
        # files: list of NemoVFSFile
        if len(files) != 1:
            return

        file = files[0]
        if file.get_uri_scheme() != 'file':
            return

        filename = urllib.unquote(file.get_uri()[7:])

        #GUI
        locale.setlocale(locale.LC_ALL, '')
        gettext.bindtextdomain("nemo-extensions")
        gettext.textdomain("nemo-extensions")
        _ = gettext.gettext

        self.property_label = Gtk.Label(_('Audio'))
        self.property_label.show()

        self.builder = Gtk.Builder()
        self.builder.set_translation_domain('nemo-extensions')
        self.builder.add_from_file("/usr/share/nemo-python/extensions/nemo-audio-tab.glade")

        #connect signals to python methods
        self.builder.connect_signals(self)

        #connect gtk objects to python variables
        for obj in self.builder.get_objects():
            if issubclass(type(obj), Gtk.Buildable):
                name = Gtk.Buildable.get_name(obj)
                setattr(self, name, obj)

        # set defaults to blank to prevent nonetype errors
        file.add_string_attribute('title', '')
        file.add_string_attribute('album', '')
        file.add_string_attribute('artist', '')
        file.add_string_attribute('albumartist', '')
        file.add_string_attribute('tracknumber', '')
        file.add_string_attribute('genre', '')
        file.add_string_attribute('date', '')
        file.add_string_attribute('bitrate', '')
        file.add_string_attribute('samplerate', '')
        file.add_string_attribute('length', '')
        file.add_string_attribute('encodedby', '')
        file.add_string_attribute('copyright', '')

        no_info = _("No Info")
        mutaFile = mutagen.File(filename)

        #MP3
        if file.is_mime_type('audio/mpeg'):
            # attempt to read ID3 tag
            try:
                audio = EasyID3(filename)
                # sometimes the audio variable will not have one of these items defined, that's why
                # there is this long try / except attempt
                try: file.add_string_attribute('title', audio["title"][0])
                except: file.add_string_attribute('title', no_info)
                try: file.add_string_attribute('album', audio["album"][0])
                except: file.add_string_attribute('album', no_info)
                try: file.add_string_attribute('artist', audio["artist"][0])
                except: file.add_string_attribute('artist', no_info)
                try: file.add_string_attribute('albumartist', audio["performer"][0])
                except: file.add_string_attribute('albumartist', no_info)
                try: file.add_string_attribute('tracknumber', audio["tracknumber"][0])
                except: file.add_string_attribute('tracknumber', no_info)
                try: file.add_string_attribute('genre', audio["genre"][0])
                except: file.add_string_attribute('genre', no_info)
                try: file.add_string_attribute('date', audio["date"][0])
                except: file.add_string_attribute('date', no_info)
                try: file.add_string_attribute('encodedby', audio["encodedby"][0])
                except: file.add_string_attribute('encodedby', no_info)
                try: file.add_string_attribute('copyright', audio["copyright"][0])
                except: file.add_string_attribute('copyright', no_info)
            except:
                # [SabreWolfy] some files have no ID3 tag and will throw this exception:
                file.add_string_attribute('title', no_info)
                file.add_string_attribute('album', no_info)
                file.add_string_attribute('artist', no_info)
                file.add_string_attribute('albumartist', no_info)
                file.add_string_attribute('tracknumber', no_info)
                file.add_string_attribute('genre', no_info)
                file.add_string_attribute('date', no_info)
                file.add_string_attribute('encodedby', no_info)
                file.add_string_attribute('copyright', no_info)

                # try to read MP3 information (bitrate, length, samplerate)
            try:
                mpinfo = MPEGInfo (filename)
                file.add_string_attribute('bitrate', str(mpinfo.bitrate/1000) + " Kbps")

                file.add_string_attribute('samplerate', str(mpinfo.sample_rate) + " Hz")
                # [SabreWolfy] added consistent formatting of times in format hh:mm:ss
                mp3length = "%02i:%02i:%02i" % ((int(mpinfo.length/3600)), (int(mpinfo.length/60%60)), (int(mpinfo.length%60)))
                file.add_string_attribute('length', mp3length)
            except:
                file.add_string_attribute('bitrate', no_info)
                file.add_string_attribute('length', no_info)
                file.add_string_attribute('samplerate', no_info)

        #FLAC
        if file.is_mime_type('audio/flac'):
            try:
                audio = FLAC(filename)
                # sometimes the audio variable will not have one of these items defined, that's why
                # there is this long try / except attempt
                try: file.add_string_attribute('title', audio["title"][0])
                except: file.add_string_attribute('title', no_info)
                try: file.add_string_attribute('album', audio["album"][0])
                except: file.add_string_attribute('album', no_info)
                try: file.add_string_attribute('artist', audio["artist"][0])
                except: file.add_string_attribute('artist', no_info)
                try: file.add_string_attribute('albumartist', audio["albumartist"][0]) # this tag is different then mp3s
                except: file.add_string_attribute('albumartist', no_info)
                try: file.add_string_attribute('tracknumber', audio["tracknumber"][0])
                except: file.add_string_attribute('tracknumber', no_info)
                try: file.add_string_attribute('genre', audio["genre"][0])
                except: file.add_string_attribute('genre', no_info)
                try: file.add_string_attribute('date', audio["date"][0])
                except: file.add_string_attribute('date', no_info)
                try: file.add_string_attribute('encodedby', audio["encoded-by"][0]) # this tag is different then mp3s
                except: file.add_string_attribute('encodedby', no_info)
                try: file.add_string_attribute('copyright', audio["copyright"][0])
                except: file.add_string_attribute('copyright', no_info)
            except:
                # [SabreWolfy] some files have no ID3 tag and will throw this exception:
                file.add_string_attribute('title', no_info)
                file.add_string_attribute('album', no_info)
                file.add_string_attribute('artist', no_info)
                file.add_string_attribute('albumartist', no_info)
                file.add_string_attribute('tracknumber', no_info)
                file.add_string_attribute('genre', no_info)
                file.add_string_attribute('date', no_info)
                file.add_string_attribute('encodedby', no_info)
                file.add_string_attribute('copyright', no_info)

                # try to read the FLAC information (length, samplerate)
            try:
                fcinfo = StreamInfo (filename)

                file.add_string_attribute('samplerate', str(fcinfo.sample_rate) + " Hz")

                # [SabreWolfy] added consistent formatting of times in format hh:mm:ss
                flaclength = "%02i:%02i:%02i" % ((int(mutaFile.info.length/3600)), (int(mutaFile.info.length/60%60)), (int(mutaFile.info.length%60)))
                file.add_string_attribute('length', flaclength)
                file.add_string_attribute('bitrate', no_info) # flac doesn't really have a bitrate
            except:
                file.add_string_attribute('bitrate', no_info)
                file.add_string_attribute('length', no_info)
                file.add_string_attribute('samplerate', no_info)

        self.builder.get_object("title_text").set_label(file.get_string_attribute('title'))
        self.builder.get_object("album_text").set_label(file.get_string_attribute('album'))
        self.builder.get_object("album_artist_text").set_label(file.get_string_attribute('albumartist'))
        self.builder.get_object("artist_text").set_label(file.get_string_attribute('artist'))
        self.builder.get_object("genre_text").set_label(file.get_string_attribute('genre'))
        self.builder.get_object("year_text").set_label(file.get_string_attribute('date'))
        self.builder.get_object("track_number_text").set_label(file.get_string_attribute('tracknumber'))
        self.builder.get_object("sample_rate_text").set_label(file.get_string_attribute('samplerate'))
        self.builder.get_object("length_number").set_label(file.get_string_attribute('length'))
        self.builder.get_object("bitrate_number").set_label(file.get_string_attribute('bitrate'))
        self.builder.get_object("encoded_by_text").set_label(file.get_string_attribute('encodedby'))
        self.builder.get_object("copyright_text").set_label(file.get_string_attribute('copyright'))

        if file.is_mime_type('audio/mpeg') or file.is_mime_type('audio/flac'):
            return Nemo.PropertyPage(name="NemoPython::audio", label=self.property_label, page=self.mainWindow),

    def get_name_and_desc(self):
        return [("Nemo Audio Tab:::View audio tag information from the properties tab")]
