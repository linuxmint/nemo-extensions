#!/usr/bin/python3
"""nemo-metadata-tab -- Nemo extension, adding a property page for media files.

Copyright (C) 2015  Eduard Dopler <kontakt@eduard-dopler.de>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

import locale
import gettext
import time
import subprocess
import signal
import sys
from collections import OrderedDict
from tempfile import NamedTemporaryFile
from urllib.parse import unquote

from gi.repository import GObject, Gtk, Nemo, GdkPixbuf, Pango, GLib
import taglib
# The current Nemo implementation has built-in EXIF support,
# toggle our own parsing here:
READ_IMAGE = True
if READ_IMAGE:
    sys.path.append("/usr/share/nemo-metadata-tab")
    try:
        import exifread
    except ImportError:
        READ_IMAGE = False
        print("nemo-metadata-tab: exifread not found. Image support disabled")
# my 2 cents on MediaInfoDLL:
# I know it is in bad shape, to put it mildly. But it is the only library I
# could find to access video metadata in Python 3. Ideas are welcome!
try:
    import MediaInfoDLL3
    READ_VIDEO = True
except ImportError:
    READ_VIDEO = False
    print("nemo-metadata-tab: MediaInfoDLL3 not found. Video support disabled")


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


TEXTDOMAIN = "nemo-metadata-tab"


def init_locale():
    """Initialize internationalization."""
    locale.setlocale(locale.LC_ALL, "")
    gettext.bindtextdomain(TEXTDOMAIN)
    gettext.textdomain(TEXTDOMAIN)


def _(message):
    """Return translated version of message.

    As the textdomain could have been changed by other (nemo-extension)
    scripts, this function checks whether it is set correctly.
    """
    if gettext.textdomain() != TEXTDOMAIN:
        init_locale()
    return gettext.gettext(message)


# extension's name and short description
NAME_DESC = "Nemo Metadata Tab:::" + _("Show property page for media files")

# tab labels by mime type
TAB_LABEL_AUDIO = _("Audio")
TAB_LABEL_VIDEO = _("Video")
TAB_LABEL_IMAGE = _("Image")
TAB_LABEL_PDF = "PDF"
# internal name for this property page
PAGE_NAME = "NemoPython::metadata"
# icon size
ICON_SIZE = 128

# default tags by type
# (will be translated and displayed, if present,
# in this ordered and before any other tags)
DEFAULT_TAGS_AUDIO = OrderedDict(
    (
        ("TITLE", _("Title")),
        ("ARTIST", _("Artist")),
        ("PERFORMER", _("Artist")),
        ("ALBUM_ARTIST", _("Album Artist")),
        ("ALBUM", _("Album")),
        ("DISCNUMBER", _("Disc No.")),
        ("TRACKNUMBER", _("Track No.")),
        ("TRACKTOTAL", _("Total Tracks")),
        ("DATE", _("Year")),
        ("GENRE", _("Genre")),
        ("COMMENT", _("Comment")),
        ("COPYRIGHT", _("Copyright")),
        ("*LENGTH", _("Length")),
        ("*BITRATE", _("Bit Rate")),
        ("*CHANNELS", _("Channels")),
        ("*SAMPLERATE", _("Sampling Rate"))
    )
)
DEFAULT_TAGS_VIDEO = OrderedDict(
    (
        ("General Format", _("Container Format")),
        ("General Duration", _("Length")),
        ("General Overall bit rate", _("Overall Bit Rate")),
        ("General Maximum Overall bit rate", _("Maximum Overall Bit Rate")),
        ("General Movie name", _("Movie Name")),
        ("General Performer", _("Performer")),
        ("General Comment", _("Comment")),
        ("General Recorded date", _("Recorded Date")),
        ("General Encoded date", _("Encoded Date")),
        ("Video Format", _("Video Format")),
        ("Video Codec", _("Video Codec")),
        ("Video Codec ID/Hint", _("Video Codec (hint)")),
        ("Video Codec ID/Info", _("Video Codec (info)")),
        ("Video Bit rate", _("Video Bit Rate")),
        ("Video Width", _("Width")),
        ("Video Height", _("Height")),
        ("Video Display aspect ratio", _("Display Aspect Ratio")),
        ("Video Frame rate", _("Frame Rate")),
        ("Video Scan type", _("Scan Type")),
        ("Video Color space", _("Color Space")),
        ("Video Chroma subsampling", _("Chroma Subsampling")),
        ("Video Language", _("Video Language")),
        ("Audio Format", _("Audio Format")),
        ("Audio Mode", _("Audio Mode")),
        ("Audio Bit rate", _("Audio Bit Rate")),
        ("Audio Channel(s)", _("Channels")),
        ("Audio Sampling rate", _("Sampling Rate")),
        ("Audio Language", _("Audio Language")),
    )
)
DEFAULT_TAGS_IMAGE = OrderedDict(
    (
        ("DateTimeOriginal", _("Date Taken")),
        ("DateTimeDigitized", _("Date Digitized")),
        ("DateTime", _("Date")),
        ("ExifImageWidth", _("Width")),
        ("ExifImageLength", _("Height")),
        ("Make", _("Camera Brand")),
        ("Model", _("Camera Model")),
        # the following 3 values will be combined in "Resolution":
        ("XResolution", _("Resolution")),
        ("YResolution", _("Resolution")),
        ("ResolutionUnit", _("Resolution")),
        ("Resolution", _("Resolution")),
        # the following 2 values will be combined in "FocalLength":
        ("FocalLength", _("Focal Length")),
        ("FocalLengthIn35mmFilm", _("Focal Length")),
        ("ExposureTime", _("Exposure Time")),
        ("FNumber", _("Aperture Value")),
        ("ISOSpeedRatings", _("ISO Speed Rating")),
        ("MeteringMode", _("Metering Mode")),
        ("WhiteBalance", _("White Balance")),
        ("Flash", _("Flash")),
        ("ExposureProgram", _("Exposure Program")),
        ("ExposureMode", _("Exposure Mode")),
        ("Software", _("Software")),
        ("ColorSpace", _("Color Space")),
        ("Contrast", _("Contrast")),
        ("Saturation", _("Saturation")),
        ("Sharpness", _("Sharpness")),
        ("Orientation", _("Orientation"))
    )
)
DEFAULT_TAGS_IMAGE_STUB = OrderedDict(
    (
        ("Width", _("Width")),
        ("Height", _("Height")),
        ("Title", _("Title")),
        ("Author", _("Author")),
        ("Description", _("Description")),
        ("Copyright", _("Copyright")),
        ("Creation Time", _("Created")),
        ("Software", _("Software")),
        ("Disclaimer", _("Disclaimer")),
        ("Warning", _("Warning")),
        ("Source", _("Source")),
        ("Comment", _("Comment"))
    )
)
DEFAULT_TAGS_PDF = OrderedDict(
    (
        ("Title", _("Title")),
        ("Subject", _("Subject")),
        ("Keywords", _("Keywords")),
        ("Author", _("Author")),
        ("Creator", _("Creator")),
        ("Producer", _("Producer")),
        ("CreationDate", _("Created")),
        ("ModDate", _("Modified")),
        ("Tagged", _("Tagged")),
        ("Form", _("Form")),
        ("Pages", _("Pages")),
        ("Encrypted", _("Encrypted")),
        ("Page size", _("Page Size")),
        ("Page rot", _("Page Rotation")),
        ("Optimized", _("Optimized")),
        ("PDF version", _("PDF Version"))
    )
)

# EXIF translations
# (These are listed here for the single purpose to be found by translation
# software as in the further code there are accessed mostly indirectly.)
(
    # FocalLength
    _("{focal_35mm} (35mm film), {focal_lense} (lens)"),
    # Flash
    _("Flash"),
    _("Flash did not fire"),
    _("Flash fired"),
    _("Strobe return light not detected"),
    _("Strobe return light detected"),
    _("return light detected"),
    _("return light not detected"),
    _("compulsory flash mode"),
    _("auto mode"),
    _("No flash function"),
    _("red-eye reduction mode"),
    # Orientation
    _("Orientation"),
    _("Horizontal (normal)"),
    _("Mirrored horizontal"),
    _("Rotated 180"),
    _("Mirrored vertical"),
    _("Mirrored horizontal then rotated 90 CCW"),
    _("Rotated 90 CW"),
    _("Mirrored horizontal then rotated 90 CW"),
    _("Rotated 90 CC"),
    # YCbCrPositioning
    _("YCbCrPositioning"),
    _("Centered"),
    _("Co-sited"),
    # ExposureProgram
    _("ExposureProgram"),
    _("Unidentified"),
    _("Manual"),
    _("Program Normal"),
    _("Aperture Priority"),
    _("Shutter Priority"),
    _("Program Creative"),
    _("Program Action"),
    _("Portrait Mode"),
    _("Landscape Mode"),
    # MeteringMode
    _("MeteringMode"),
    _("Unidentified"),
    _("Average"),
    _("CenterWeightedAverage"),
    _("Spot"),
    _("MultiSpot"),
    _("Pattern"),
    _("Partial"),
    _("other"),
    # LightSource (most common)
    _("LightSource"),
    _("Unknown"),
    _("Daylight"),
    _("Fluorescent"),
    _("Flash"),
    # SensingMethod
    _("SensingMethod"),
    _("Not defined"),
    _("One-chip color area"),
    _("Two-chip color area"),
    _("Three-chip color area"),
    _("Color sequential area"),
    _("Trilinear"),
    _("Color sequential linear"),
    # FileSource
    _("FileSource"),
    _("Film Scanner"),
    _("Reflection Print Scanner"),
    _("Digital Camera"),
    # SceneType
    _("SceneType"),
    _("Directly Photographed"),
    # CustomRendered
    _("CustomRendered"),
    _("Normal"),
    _("Custom"),
    # ExposureMode
    _("ExposureMode"),
    _("Auto Exposure"),
    _("Manual Exposure"),
    _("Auto Bracket"),
    # WhiteBalance
    _("WhiteBalance"),
    _("Auto"),
    _("Manual"),
    # SceneCaptureType
    _("SceneCaptureType"),
    _("Standard"),
    _("Landscape"),
    _("Portrait"),
    _("Night"),
    # GainControl
    _("GainControl"),
    _("None"),
    _("Low gain up"),
    _("High gain up"),
    _("Low gain down"),
    _("High gain down"),
    # Contrast, Saturation, Sharpness
    _("Contrast"),
    _("Saturation"),
    _("Sharpness"),
    _("Normal"),
    _("Soft"),
    _("Hard")
)


class NemoMetadataTab(GObject.GObject,
                      Nemo.PropertyPageProvider,
                      Nemo.NameAndDescProvider):

    """Display metadata of media files in Nemo's property page.

    The current implementation displays metadata of audio, video, image
    and PDF files.
    """

    def __init__(self):
        """Load MediaInfo library in advance."""
        self.media_info = MediaInfoDLL3.MediaInfo()

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [NAME_DESC]

    def get_property_pages(self, files):
        """Construct a Nemo property page and return it."""
        # show tab only for one file
        if len(files) != 1:
            return
        file_object = files[0]

        if file_object.get_uri_scheme() != "file":
            return
        filename = unquote(file_object.get_uri()[7:])
        mime_type = file_object.get_mime_type()

        # load metadata by file type
        # Audio
        if mime_type.startswith("audio"):
            label = TAB_LABEL_AUDIO
            tagdata = read_audio_data(filename)
            default_tags = DEFAULT_TAGS_AUDIO
            # cover art
            if mime_type.endswith("mpeg"):
                icon = read_id3_art(filename)
            elif mime_type.endswith("flac"):
                icon = read_flac_art(filename)
            else:
                icon = None
        # Video
        elif mime_type.startswith("video"):
            if not READ_VIDEO:
                return
            label = TAB_LABEL_VIDEO
            tagdata = read_video_data(filename, self.media_info)
            default_tags = DEFAULT_TAGS_VIDEO
            icon = Gtk.Image.new_from_icon_name("video", Gtk.IconSize.DIALOG)
        # Image
        elif mime_type.startswith("image"):
            if not READ_IMAGE:
                return
            label = TAB_LABEL_IMAGE
            # try first with exifread
            tagdata = read_image_data(filename)
            default_tags = DEFAULT_TAGS_IMAGE
            # or else at least some info from GdkPixbuf
            if not tagdata:
                tagdata = read_image_stub(filename)
                default_tags = DEFAULT_TAGS_IMAGE_STUB
            icon = Gtk.Image.new_from_icon_name("image", Gtk.IconSize.DIALOG)
        # PDF
        elif mime_type == "application/pdf":
            label = TAB_LABEL_PDF
            tagdata = read_pdf_data(filename)
            default_tags = DEFAULT_TAGS_PDF
            icon = Gtk.Image.new_from_icon_name("application-pdf",
                                                Gtk.IconSize.DIALOG)
        # unsupported mime
        else:
            return

        if tagdata:
            return self._build_gui(label, tagdata, default_tags, icon)

    def _build_gui(self, label, tagdata, default_tags=None, icon=None):
        """Build and return GUI based on the tagdata retrieved before.

        label -- tab label/title text widget
        tagdata -- data to be displayed
        default_tags -- dict of {tags : translated_tags} (will be listed first)
        icon -- image widget to be
        """
        tab_label = Gtk.Label(label)
        tab_label.show()

        tab = Gtk.ScrolledWindow()
        holder = Gtk.VBox(valign=Gtk.Align.START)
        tab.add(holder)
        grid_default_tags = Gtk.Grid(
            orientation=Gtk.Orientation.VERTICAL,
            column_homogeneous=False,
            column_spacing=15,
            row_spacing=10,
            border_width=20)
        holder.pack_start(grid_default_tags, True, False, 0)

        details = Gtk.Expander(label=_("More…"))
        holder.pack_start(details, True, False, 0)
        grid_details = Gtk.Grid(
            orientation=Gtk.Orientation.VERTICAL,
            column_homogeneous=False,
            column_spacing=15,
            row_spacing=10,
            border_width=20)
        details.add(grid_details)

        # put tagdata in a grid model,
        # leave out empty tags and
        # try to use translated versions of tags (as defined in default_tags)
        # non-default tags are hidden in an expander
        n_rows = 0
        for tagname, data in tagdata.items():
            if not data:
                continue
            if default_tags and tagname in default_tags:
                nice_tagname = default_tags[tagname] + ":"
                grid = grid_default_tags
                n_rows += 1
            else:
                nice_tagname = tagname + ":"
                grid = grid_details
            label_name = Gtk.Label(label=nice_tagname,
                                   halign=Gtk.Align.START,
                                   valign=Gtk.Align.START)
            grid.add(label_name)
            tagvalue = "".join(data)
            label_data = Gtk.Label(label=tagvalue,
                                   halign=Gtk.Align.START,
                                   valign=Gtk.Align.START,
                                   hexpand=True,
                                   selectable=True)
            label_data.set_line_wrap(True)
            label_data.set_ellipsize(Pango.EllipsizeMode.END)
            grid.attach_next_to(label_data, label_name,
                                Gtk.PositionType.RIGHT, 1, 1)

        # add icon, if any
        if icon:
            grid_default_tags.attach_next_to(
                icon,
                grid_default_tags.get_child_at(1, 0),
                Gtk.PositionType.RIGHT, 1, n_rows)
            icon.set_valign(Gtk.Align.START)

        # Show details expander only when there are any
        if not grid_details.get_children():
            details.destroy()

        tab.show_all()
        return Nemo.PropertyPage(name=PAGE_NAME, label=tab_label, page=tab),


def read_audio_data(filename):
    """Read and return audio tags (incl. metadata) from audio files.

    (This method makes use of the Python3 bindings of taglib.)
    """
    try:
        f = taglib.File(filename)
    except OSError:
        return

    tagdata = OrderedDict.fromkeys(DEFAULT_TAGS_AUDIO)
    tagdata.update(f.tags)

    # add meta information
    metadata = {
        "*LENGTH": sec2min(f.length),
        "*BITRATE": "%s kbit/s" % str(f.bitrate),
        "*CHANNELS": str(f.channels),
        "*SAMPLERATE": "%s Hz" % str(f.sampleRate)
    }
    tagdata.update(metadata)

    return tagdata


def read_video_data(filename, media_info):
    """Read and return video metadata.

    In the current implementation all metadata found by MediaInfoDLL3
    will be returned and no translation takes place as English media
    terms are more common than translated interpretations.

    (This method makes use of the Python3 bindings of MediaInfoDLL3.)
    """
    open_result = media_info.Open(filename)
    if not open_result:
        return
    raw_tag_data = media_info.Inform()
    media_info.Close()

    tagdata = OrderedDict.fromkeys(DEFAULT_TAGS_VIDEO)

    for line in raw_tag_data.split("\n"):
        tagname, x, tagvalue = line.partition(":")
        # if this line is a heading, use it as prefix to prevent overwriting
        if tagname and not tagvalue:
            prefix = tagname.strip()
        tagname = prefix + " " + tagname.strip()
        tagvalue = tagvalue.strip()
        # don't add unnecessary file path and size
        if ((tagname == "General Complete name" or
             tagname == "General File size")):
            continue
        # translate "pixels"
        if tagname in ("Video Width", "Video Height"):
            tagvalue = tagvalue.replace("pixels", _("%s pixels")[3:])
        if not tagvalue:
            continue

        tagdata[tagname] = tagvalue

    # combine bit rate mode with bit rate (for all prefixes)
    prefixes = ("General Overall b", "Video B", "Audio B")
    for prefix in prefixes:
        tagname_bitrate_mode = prefix + "it rate mode"
        tagname_bitrate = prefix + "it rate"
        if ((tagname_bitrate_mode in tagdata and
             tagdata[tagname_bitrate_mode] and
             tagname_bitrate in tagdata and
             tagdata[tagname_bitrate])):
            tagdata[tagname_bitrate] = "{bitrate} ({bitrate_mode})".format(
                bitrate=tagdata[tagname_bitrate],
                bitrate_mode=tagdata[tagname_bitrate_mode].lower())
            del tagdata[tagname_bitrate_mode]

    # combine codec hint and info into "Codec"
    prefixes = ("Video Codec", "Audio Codec")
    for prefix in prefixes:
        tagname_hint = prefix + " ID/Hint"
        tagname_info = prefix + " ID/Info"
        tagname_codec = prefix
        if ((tagname_hint in tagdata and
             tagdata[tagname_hint] and
             tagname_info in tagdata and
             tagdata[tagname_info])):
            tagdata[tagname_codec] = "{info} ({hint})".format(
                info=tagdata[tagname_info],
                hint=tagdata[tagname_hint])
            del tagdata[tagname_hint]
            del tagdata[tagname_info]

    # combine frame rate mode with frame rate
    tagname_framerate_mode = "Frame rate mode"
    tagname_framerate = "Frame rate"
    if ((tagname_framerate_mode in tagdata and
         tagdata[tagname_framerate_mode] and
         tagname_framerate in tagdata and
         tagdata[tagname_framerate])):
        tagdata[tagname_framerate] = "{framerate} ({framerate_mode})".format(
            framerate=tagdata[tagname_framerate],
            framerate_mode=tagdata[tagname_framerate_mode].lower())
        del tagdata[tagname_framerate_mode]

    # combine format with format profile
    prefixes = ("Video", "Audio")
    for prefix in prefixes:
        tagname_format = prefix + " Format"
        tagname_format_profile = prefix + " Format profile"
        if ((tagname_format_profile in tagdata and
             tagdata[tagname_format_profile] and
             tagname_format in tagdata and
             tagdata[tagname_format])):
            tagdata[tagname_format] = "{format} ({format_profile})".format(
                format=tagdata[tagname_format],
                format_profile=tagdata[tagname_format_profile])
            del tagdata[tagname_format_profile]

    return tagdata


def read_image_data(filename):
    """Read and return image metadata.

    Some values will be combined and all common EXIF values will be
    translated into user's locale.

    (This method makes use of the Python3 library exifread.)
    """
    with open(filename, "rb") as f:
        tags_raw = exifread.process_file(f, details=False)

    if not tags_raw:
        return

    tagdata = OrderedDict.fromkeys(DEFAULT_TAGS_IMAGE)
    # Values returned by exifread have to be converted to strings in order to
    # be readable.
    # While we're at it: sort by key, skip some tags
    # and remove prepended category from tags.
    for tagname, tagvalue in sorted(tags_raw.items()):
        if any((tagname.startswith("Thumbnail"),
                tagname.startswith("Interoper"),
                tagname.startswith("EXIF SubSec"),
                tagname.endswith("Offset"))):
            continue
        x, x, tagname = tagname.partition(" ")
        tagdata[tagname] = str(tagvalue)

    # use only one DateTime info (by priority)
    # and translate into locale's representation
    if tagdata["DateTimeOriginal"]:
        del tagdata["DateTimeDigitized"]
        del tagdata["DateTime"]
        tagdata["DateTimeOriginal"] = translate_exif_date(
            tagdata["DateTimeOriginal"])
    else:
        if tagdata["DateTimeDigitized"]:
            del tagdata["DateTime"]
            tagdata["DateTimeDigitized"] = translate_exif_date(
                tagdata["DateTimeDigitized"])
        else:
            tagdata["DateTime"] = translate_exif_date(tagdata["DateTime"])

    # combine "Resolution" values
    if ((tagdata["XResolution"] and
         tagdata["YResolution"] and
         tagdata["ResolutionUnit"])):
        x_res = tagdata["XResolution"]
        y_res = tagdata["YResolution"]
        if x_res == y_res:
            resolution = x_res
        else:
            resolution = x_res + "x" + y_res
        unit = tagdata["ResolutionUnit"]
        unit = unit.replace("Pixels/Inch", "ppi").replace("Dots/Inch", "dpi")
        tagdata["Resolution"] = "{resolution} {unit}".format(
            resolution=resolution, unit=unit)
        del tagdata["XResolution"]
        del tagdata["YResolution"]
        del tagdata["ResolutionUnit"]

    # combine "FocalLength" values
    if ((tagdata["FocalLength"] and
         tagdata["FocalLengthIn35mmFilm"])):
        focal_length = "{focal_35mm} (35mm film), {focal_lense} (lens)"
        tagdata["FocalLength"] = _(focal_length).format(
            focal_35mm=tagdata["FocalLengthIn35mmFilm"],
            focal_lense=tagdata["FocalLength"])
        del tagdata["FocalLengthIn35mmFilm"]

    # add "pixels" to width and height
    if ((tagdata["ExifImageWidth"] and
         tagdata["ExifImageLength"])):
        tagdata["ExifImageWidth"] = (_("%s pixels") %
                                     tagdata["ExifImageWidth"])
        tagdata["ExifImageLength"] = (_("%s pixels") %
                                      tagdata["ExifImageLength"])

    # add "Sec." to exposure time
    if tagdata["ExposureTime"]:
        tagdata["ExposureTime"] = (_("%s sec.") % tagdata["ExposureTime"])

    # calculate f-number
    if tagdata["FNumber"]:
        first, x, second = tagdata["FNumber"].partition("/")
        fnumber = float(first) / float(second)
        tagdata["FNumber"] = "f/" + str(fnumber)

    # translate some words
    translatable_tags = (
        "Orientation", "YCbCrPositioning", "ExposureProgram", "Sharpness",
        "MeteringMode", "LightSource", "SensingMethod", "FileSource", "Flash",
        "SceneType", "CustomRendered", "ExposureMode", "WhiteBalance",
        "SceneCaptureType", "GainControl", "Contrast", "Saturation"
    )

    for tagname in tagdata:
        if tagname in translatable_tags:
            tagdata[tagname] = translate_exif_words(tagname, tagdata[tagname])

    return tagdata


def read_image_stub(filename):
    """Read and return image metadata provided by GdkPixbuf.

    This is usually less extensive than EXIF data but provides basic
    information for more image types.
    """
    try:
        pixbuf = GdkPixbuf.Pixbuf.new_from_file(filename)
    except GLib.GError:
        return

    tagdata = OrderedDict.fromkeys(DEFAULT_TAGS_IMAGE_STUB)

    for tagname in DEFAULT_TAGS_IMAGE_STUB:
        pixbuf_option = "tEXt::" + tagname
        tagdata[tagname] = pixbuf.get_option(pixbuf_option)

    tagdata["Width"] = pixbuf.get_width()
    tagdata["Height"] = pixbuf.get_height()

    # add "pixels" to width and height
    if ((tagdata["Width"] and
         tagdata["Height"])):
        tagdata["Width"] = (_("%s pixels") % tagdata["Width"])
        tagdata["Height"] = (_("%s pixels") % tagdata["Height"])

    return tagdata


def read_pdf_data(filename):
    """Read and return PDF metadata.

    (This method makes use of a subprocess call to the binary
    "pdfinfo" by poppler and processes its output. E.g. words and
    dates are translated.)
    """
    try:
        pdfinfo = subprocess.check_output(["pdfinfo", filename],
                                          stderr=subprocess.DEVNULL)
        pdfinfo = pdfinfo.decode()
    except (subprocess.CalledProcessError, UnicodeError):
        return

    tagdata = OrderedDict.fromkeys(DEFAULT_TAGS_PDF)

    for line in pdfinfo.split("\n"):
        tagname, x, tagvalue = line.partition(":")
        tagvalue = tagvalue.strip()
        # don't add unnecessary file size
        if tagname == "File size":
            continue
        # convert all dates to locale version
        if tagname.endswith("Date"):
            tagvalue = translate_poppler_date(tagvalue)
        # add nicer spacing/seperators in encryption list
        elif tagname == "Encrypted" and tagvalue != "no":
            tagvalue = tagvalue.replace(" ", ", ")
            tagvalue = tagvalue.replace(":", ": ")
            tagvalue = tagvalue.replace(", (", " (")
        elif tagname == "Page rot":
            tagvalue += "°"
        # translate some values
        if tagname in ("Tagged", "Form", "Encrypted", "Optimized"):
            tagvalue = translate_poppler_words(tagvalue)

        tagdata[tagname] = tagvalue

    return tagdata


def read_id3_art(filename):
    """Read and return (cover) art from ID3 files, if any.

    See the ID3 specification for details [https://id3.org/id3v2.3.0].
    """
    with open(filename, "r+b") as f:
        if not f.peek(3).startswith(b"ID3"):
            return None
        f.seek(5)  # start of ID3v2 flags
        # extended header present?
        has_extended = bytes_to_int(f.read(1)) >= 64
        total_tag_size = safeint_to_int(f.read(4))
        # skip extended header
        if has_extended:
            extended_header_size = safeint_to_int(f.read(4))
            f.seek(extended_header_size, 1)
            # skip padding
            while f.peek(1)[0].startswith(b"\x00"):
                f.seek(1, 1)
        # skip all non-APIC frames
        while not f.peek(4).startswith(b"APIC"):
            f.seek(4, 1)  # skip ID
            frame_size = safeint_to_int(f.read(4))
            f.seek(2, 1)  # skip flags
            f.seek(frame_size, 1)
            # do not exceed total tag size
            if f.tell() > total_tag_size + 10:
                return
        # at start of APIC frame now
        f.seek(4, 1)  # skip ID
        apic_size = safeint_to_int(f.read(4))
        f.seek(2, 1)  # skip flags
        apic_frame_start = f.tell()
        f.seek(1, 1)  # skip encoding
        # find end of mime type
        while f.read(1) != b"\x00":
            pass
        f.seek(1, 1)  # skip picture type
        # find end of description
        while f.read(1) != b"\x00":
            pass
        # read picture
        picture_size = apic_size - f.tell() + apic_frame_start
        picture = f.read(picture_size)
    # create a temporary file so GtkPixbuf can read it
    with NamedTemporaryFile("w+b") as temp_pic_file:
        temp_pic_file.write(picture)
        pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_size(temp_pic_file.name,
                                                        ICON_SIZE,
                                                        ICON_SIZE)
        widget = Gtk.Image.new_from_pixbuf(pixbuf)
        widget.set_padding(2, 2)
        frame = Gtk.Frame(shadow_type=Gtk.ShadowType.ETCHED_OUT)
        frame.add(widget)
        return frame


def read_flac_art(filename):
    """Read and return (cover) art from FLAC files, if any.

    See the FLAC specification for details [https://xiph.org/flac/format.html].
    """
    with open(filename, "r+b") as f:
        if not f.peek(4).startswith(b"fLaC"):
            return None
        # skip STREAMINFO block
        f.seek(42)
        # skip all non-PICTURE blocks
        while not f.peek(1).startswith(b"\x06"):
            # last metadata block?
            if bytes_to_int(f.read(1)) >= 128:
                return None
            block_size = bytes_to_int(f.read(3))
            f.seek(block_size, 1)
        # at start of PICTURE block now
        # skip block size and picture type
        f.seek(8, 1)
        # skip mime
        mime_size = bytes_to_int(f.read(4))
        f.seek(mime_size, 1)
        # skip description
        description_size = bytes_to_int(f.read(4))
        f.seek(description_size, 1)
        # skip more picture info
        f.seek(16, 1)
        # read picture
        picture_size = bytes_to_int(f.read(4))
        picture = f.read(picture_size)
    # create a temporary file so GtkPixbuf can read it
    with NamedTemporaryFile("w+b") as temp_pic_file:
        temp_pic_file.write(picture)
        pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_size(temp_pic_file.name,
                                                        ICON_SIZE,
                                                        ICON_SIZE)
        widget = Gtk.Image.new_from_pixbuf(pixbuf)
        widget.set_padding(2, 2)
        frame = Gtk.Frame(shadow_type=Gtk.ShadowType.ETCHED_OUT)
        frame.add(widget)
        return frame


def translate_poppler_date(english_date):
    """Translate "english date" into locale date representation.

    ("english date" means the date format used by Poppler's pdfinfo.)
    """
    locale.setlocale(locale.LC_TIME, ("en", "utf-8"))
    time_object = time.strptime(english_date, "%a %b %d %H:%M:%S %Y")
    init_locale()
    locale_time = time.strftime("%c", time_object)
    return locale_time


def translate_exif_date(exif_date):
    """Translate EXIF's date/time representation into locale's one."""
    time_object = time.strptime(exif_date, "%Y:%m:%d %H:%M:%S")
    locale_time = time.strftime("%c", time_object)
    return locale_time


def translate_exif_words(tagname, tagvalue):
    """Translate common EXIF words into user's locale.

    tagname -- the EXIF tag's name
    tagvalue -- the tag's value to be translated
    """
    # replace every part in Flash info with its translated version
    if tagname == "Flash":
        flash_parts = tagvalue.split(", ")
        tagvalue = ""
        for part in flash_parts:
            tagvalue += _(part) + ", "
        tagvalue = tagvalue.rstrip(", ")
    # all other tags can be replaced directly
    else:
        tagvalue = _(tagvalue)

    return tagvalue


def translate_poppler_words(tagvalue):
    """Translate common words printed by pdfinfo into user's locale."""
    if tagvalue == "none":
        return _("None")
    elif tagvalue == "yes":
        return _("Yes")
    elif tagvalue == "no":
        return _("No")
    else:
        # "Encrypted" value
        # multiple "yes"/"no" account for prevention of in-word substitution
        wordlist = {
            "print": _("Print"),
            "copy": _("Copy"),
            "change": _("Change"),
            "addNotes": _("Add Notes"),
            "algorithm": _("Algorithm"),
            "yes (": _("Yes") + " (",
            "no (": _("No") + " (",
            "yes,": _("Yes") + ",",
            "no,": _("No") + ","
        }
        for word in wordlist:
            tagvalue = tagvalue.replace(word, wordlist[word])
        return tagvalue


def sec2min(totalsec):
    """Convert from all-seconds into minutes:seconds representation."""
    minutes, seconds = divmod(totalsec, 60)
    return "%d:%02d" % (minutes, seconds)


def bytes_to_int(bytes_raw):
    """Convert byte string holding hex values into int."""
    num_bytes = len(bytes_raw)
    sum_int = 0
    for i in range(num_bytes):
        zeros = (num_bytes - i - 1) * "00"
        sum_int += int(hex(bytes_raw[i]) + zeros, base=16)

    return sum_int


def safeint_to_int(raw_bytes):
    """Convert Safe Integers to Integers.

    ID3 frame sizes are encoded in the Safe Integer representation.
    """
    safeint = bytes_to_int(raw_bytes)

    a = safeint & 0xff
    b = (safeint >> 8) & 0xff
    c = (safeint >> 16) & 0xff
    d = (safeint >> 24) & 0xff

    py_int = 0x0 | a
    py_int = py_int | (b << 7)
    py_int = py_int | (c << 14)
    py_int = py_int | (d << 21)

    return py_int
