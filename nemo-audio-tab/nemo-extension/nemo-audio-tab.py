#!/usr/bin/python3

# change log:
# RavetcoFX: Forked from nemo-media-columns and nemo-emblems

import gettext
import json
import locale
import os
import subprocess
from urllib import parse

from gi.repository import GObject, Gtk, Nemo


def probe(filename):
    """Run ffprobe on the specified file and return a JSON representation of the output.

    Raises:
        :class:`ffmpeg.Error`: if ffprobe returns a non-zero exit code,
            an :class:`Error` is returned with a generic error message.
            The stderr output can be retrieved by accessing the
            ``stderr`` property of the exception.
    """
    args = ['ffprobe', '-show_format', '-show_streams', '-of', 'json', filename]

    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    if p.returncode != 0:
        return None
    return json.loads(out.decode('utf-8'))


def get_meta(filename):
    """returns None if falure"""
    streams = probe(filename)
    if streams is None:
        return None
    stream = next((s for s in streams['streams'] if s['codec_type'] == 'audio'), None)
    if stream is None:
        return None

    tags = {}
    tags['duration'] = 'unknown'
    if 'duration' in stream:
        total_duration = float(stream["duration"])
        hours_ = int(total_duration // 3600)
        total_duration -= 3600*hours_
        min_ = int(total_duration // 60)
        total_duration -= 60*min_
        sec_ = int(total_duration)
        ms_ = int(1000*(total_duration-sec_))
        tags['duration'] = f'{hours_:02}:{min_:02}:{sec_:02}.{ms_} (h:min:s.ms)'

    tags['bitrate'] = 'unknown'
    if 'bit_rate' in stream:
        tags['bitrate'] = f'{float(stream["bit_rate"])/1000:.2f} (kbit/s)'

    tags['samplerate'] = 'unknown'
    if 'sample_rate' in stream:
        tags['samplerate'] = f'{stream["sample_rate"]} (Hz)'

    tags['channels'] = 'unknown'
    if 'channels' in stream:
        tags['channels'] = (
            str(stream['channels'])
            + (
                f' ({stream["channel_layout"]})'
                if 'channel_layout' in stream else ''
            )
        )

    tags['codec'] = 'unknown'
    if 'codec_long_name' in stream:
        tags['codec'] = stream['codec_long_name']
    elif 'codec_name' in stream:
        tags['codec'] = stream['codec_name']

    tags['other'] = {
        **(stream['tags'] if 'tags' in stream else {}),
        **(streams['format']['tags'] if 'tags' in streams['format'] else {}),
    }

    return tags



class AudioPropertyPage(GObject.GObject, Nemo.PropertyPageProvider, Nemo.NameAndDescProvider):

    def get_property_pages(self, files):

        # files: list of NemoVFSFile
        if len(files) != 1:
            return []

        file = files[0]
        if file.get_uri_scheme() != 'file':
            return []

        if file.is_directory():
            return []

        filename = parse.unquote(file.get_uri()[7:])

        tags = get_meta(filename)
        if tags is None:
            return []

        #GUI
        locale.setlocale(locale.LC_ALL, '')
        gettext.bindtextdomain("nemo-extensions")
        gettext.textdomain("nemo-extensions")
        _ = gettext.gettext

        self.property_label = Gtk.Label(_('Audio'))
        self.property_label.show()

        self.builder = Gtk.Builder()
        self.builder.set_translation_domain('nemo-extensions')
        self.builder.add_from_file("/usr/share/nemo-audio-tab/nemo-audio-tab.glade")

        #connect gtk objects to python variables
        for obj in self.builder.get_objects():
            if issubclass(type(obj), Gtk.Buildable):
                name = Gtk.Buildable.get_name(obj)
                setattr(self, name, obj)

        no_info = _("No Info")

        for attribute in ('duration', 'bitrate', 'samplerate', 'channels', 'codec'):
            file.add_string_attribute(attribute, tags[attribute])
        if tags['other']:
            file.add_string_attribute('other', '\n'.join(f'{key}: {val}' for key, val in tags['other'].items()))
        else:
            file.add_string_attribute('other', no_info)

        for attribute in ('duration', 'bitrate', 'samplerate', 'channels', 'codec', 'other'):
            self.builder.get_object(f'{attribute}_text').set_label(file.get_string_attribute(attribute))

        return [
            Nemo.PropertyPage(name="NemoPython::audio",
                              label=self.property_label,
                              page=self.builder_root_widget)
        ]

    def get_name_and_desc(self):
        return [("Nemo Audio Tab:::View audio tag information from the properties tab")]
