#!/usr/bin/python3
"""nemo-metadata-columns -- Nemo extension, adding columns for media metadata.

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
from urllib.parse import unquote

from gi.repository import GObject, Nemo, GdkPixbuf, GLib
# my 2 cents on MediaInfoDLL:
# I know it is in bad shape, to put it mildly. But it is the only library I
# could find to access video metadata in Python 3. Ideas are welcome!
import MediaInfoDLL3
# exifread is an external library that should come with this script
# installed in the following directory. Keep it optional, though.
READ_IMAGE = True
if READ_IMAGE:
    try:
        sys.path.append("/usr/share/nemo-metadata-columns")
        import exifread
    except ImportError:
        READ_IMAGE = False
        print("nemo-metadata-colu: exifread not found. Image support disabled")


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


TEXTDOMAIN = "nemo-metadata-columns"


def init_locale():
    """Initialize internationalization."""
    locale.setlocale(locale.LC_ALL, "")
    gettext.bindtextdomain("nemo-metadata-columns")
    gettext.textdomain("nemo-metadata-columns")


def _(message):
    """Return translated version of message.

    As the textdomain could have been changed by other (nemo-extension)
    scripts, this function checks whether it is set correctly.
    """
    if gettext.textdomain() != TEXTDOMAIN:
        init_locale()
    return gettext.gettext(message)


# tags to be shown in columns and their translations
TAGS_AUDIO_VIDEO = {
    "GENERAL DURATION": _("Length"),
    "GENERAL OVERALL BIT RATE": _("Overall Bitrate"),
    "GENERAL ALBUM": _("Album"),
    "GENERAL TRACK NAME": _("Title"),
    "GENERAL TRACK NAME/POSITION": _("Track#"),
    "GENERAL PERFORMER": _("Artist"),
    "GENERAL GENRE": _("Genre"),
    "GENERAL RECORDED DATE": _("Date"),
    "VIDEO FORMAT": _("Video Format"),
    "VIDEO CODEC ID/HINT": _("Video Codec"),
    "VIDEO BIT RATE": _("Video Bitrate"),
    "VIDEO WIDTH": _("Width"),
    "VIDEO HEIGHT": _("Height"),
    "VIDEO DISPLAY ASPECT RATIO": _("Aspect"),
    "VIDEO FRAME RATE": _("Framerate"),
    "AUDIO FORMAT": _("Audio Format"),
    "AUDIO BIT RATE": _("Audio Bitrate"),
    "AUDIO CHANNEL(S)": _("Channels"),
    "AUDIO SAMPLING RATE": _("Sample Rate"),
    "AUDIO REPLAY GAIN": _("Replay Gain"),
    "AUDIO LANGUAGE": _("Language"),
}
TAGS_IMAGE = {
    "Image Make": _("Camera Brand"),
    "Image Model": _("Camera Model"),
    "EXIF FocalLength": _("Focal Length"),
    "EXIF ExposureTime": _("Exposure Time (sec.)"),
    "EXIF FNumber": _("Aperture Value"),
    "EXIF ISOSpeedRatings": _("ISO Speed Rating"),
    "EXIF MeteringMode": _("Metering Mode"),
    "EXIF WhiteBalance": _("White Balance"),
    "EXIF Flash": _("Flash"),
    "EXIF ExposureProgram": _("Exposure Program"),
    "EXIF ExposureMode": _("Exposure Mode"),
    "Image Software": _("Software"),
    "EXIF ColorSpace": _("Color Space"),
    "EXIF Contrast": _("Contrast"),
    "EXIF Saturation": _("Saturation"),
    "EXIF Sharpness": _("Sharpness"),
    "Image Orientation": _("Orientation")
}
# hidden tags are read internally but not displayed on their own
TAGS_IMAGE_HIDDEN = {
    "EXIF ExifImageWidth",
    "EXIF ExifImageLength",
    "EXIF DateTimeOriginal",
    "EXIF DateTimeDigitized",
    "Image DateTime",
    "Image XResolution",
    "Image YResolution",
    "Image ResolutionUnit",
    "EXIF FocalLengthIn35mmFilm"
}
TAGS_IMAGE_STUB = {
    "Description": _("Description"),
    "Copyright": _("Copyright"),
    "Disclaimer": _("Disclaimer"),
    "Warning": _("Warning"),
    "Source": _("Source"),
    "Comment": _("Comment")
}
TAGS_IMAGE_STUB_HIDDEN = {
    "Title",
    "Author",
    "Creation Time",
    "Software"
}
TAGS_PDF = {
    "SUBJECT": _("Subject"),
    "KEYWORDS": _("Keywords"),
    "AUTHOR": _("Author"),
    "MODDATE": _("Modified"),
    "PAGES": _("Pages")
}
TAGS_PDF_HIDDEN = {
    "TITLE"
}

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
    # ExposureMode
    _("ExposureMode"),
    _("Auto Exposure"),
    _("Manual Exposure"),
    _("Auto Bracket"),
    # WhiteBalance
    _("WhiteBalance"),
    _("Auto"),
    _("Manual"),
    # Contrast, Saturation, Sharpness
    _("Contrast"),
    _("Saturation"),
    _("Sharpness"),
    _("Normal"),
    _("Soft"),
    _("Hard")
)


class NemoMetadataColumns(GObject.GObject,
                          Nemo.ColumnProvider,
                          Nemo.InfoProvider,
                          Nemo.NameAndDescProvider):

    """Display metadata of media files in Nemo's columns.

    The current implementation displays metadata of audio, video, image
    and PDF files.
    """

    def __init__(self):
        """Load MediaInfo library in advance."""
        self.media_info = MediaInfoDLL3.MediaInfo()

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [("Nemo Metadata Column:::" +
                 _("Show media metadata in columns"))]

    def get_columns(self):
        """Return NemoColumns for media metadata."""
        columns = []
        tags = dict(TAGS_AUDIO_VIDEO)
        tags.update(TAGS_IMAGE)
        tags.update(TAGS_PDF)

        for tagname, tag_translated in tags.items():
            attribute = tagname.lower()
            name = "NemoMetadata::%s_column" % attribute
            label = tag_translated
            columns.append(
                Nemo.Column(name=name,
                            attribute=attribute,
                            label=label,
                            description=tagname)
            )
        return tuple(columns)

    def update_file_info(self, file):
        """Read metadata and store it in file info attributes."""
        file_object = file

        if file_object.get_uri_scheme() != "file":
            return
        filename = unquote(file_object.get_uri()[7:])
        mime_type = file_object.get_mime_type()

        # load metadata by file type
        # Audio/Video
        if ((mime_type.startswith("audio") or
             mime_type.startswith("video"))):
            tagdata = read_audio_video_data(filename, self.media_info)
        # Image
        elif mime_type.startswith("image"):
            tagdata = read_image_data(filename)
            # if exifread was not successful
            # get at least some info from GdkPixbuf
            if not tagdata:
                tagdata = read_image_stub(filename)
        # PDF
        elif mime_type == "application/pdf":
            tagdata = read_pdf_data(filename)
        # unsupported mime
        else:
            return

        if not tagdata:
            return

        for tagname, data in tagdata.items():
            if not data:
                continue
            file_object.add_string_attribute(tagname.lower(), data)

        return Nemo.OperationResult.COMPLETE


def read_audio_video_data(filename, media_info):
    """Read and return video metadata defined in TAGS_AUDIO_VIDEO.

    media_info -- instance of MediaInfoDLL3.MediaInfo

    (This method makes use of the Python3 bindings of MediaInfoDLL3.)
    """
    open_result = media_info.Open(filename)
    if open_result != 1:
        return

    tagdata = {}
    raw_tag_data = media_info.Inform()

    for line in raw_tag_data.split("\n"):
        tagname, x, tagvalue = line.partition(":")
        # if this line is a heading, use it as prefix to prevent overwriting
        if tagname and not tagvalue:
            prefix = tagname.strip().upper()
        tagname = prefix + " " + tagname.strip().upper()

        tagvalue = tagvalue.strip()
        if not tagvalue:
            continue

        if tagname in TAGS_AUDIO_VIDEO:
            # unify the duration representation
            if tagname == "GENERAL DURATION":
                tagvalue = convert_duration(tagvalue)
            # translate "pixels"
            if tagname in ("VIDEO WIDTH", "VIDEO HEIGHT"):
                tagvalue = tagvalue.replace("pixels", _("%s pixels")[3:])
            # in order to avoid complicated translations (e.g. plurals, lower/
            # uppercase), delete the word from "%s channel" and "%s channels"
            if tagname == "AUDIO CHANNEL(S)":
                tagvalue = tagvalue.replace("channel", "").replace(" s", "")

            tagdata[tagname] = tagvalue

    media_info.Close()

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

    tagdata = {tagname: str(tagvalue)
               for tagname, tagvalue in tags_raw.items()
               if (tagname in TAGS_IMAGE or
                   tagname in TAGS_IMAGE_HIDDEN)}

    if not tagdata:
        return

    # use only one DateTime info (by priority)
    # but don't translate (in order to be sortable)
    # and store it in Audio's GENERAL RECORDED DATE in order to reduce columns
    date_tag = "GENERAL RECORDED DATE"
    if tagdata["EXIF DateTimeOriginal"]:
        tagdata[date_tag] = tagdata["EXIF DateTimeOriginal"]
    else:
        if tagdata["EXIF DateTimeDigitized"]:
            tagdata[date_tag] = tagdata["EXIF DateTimeDigitized"]
        else:
            tagdata[date_tag] = tagdata["Image DateTime"]
    del tagdata["EXIF DateTimeOriginal"]
    del tagdata["EXIF DateTimeDigitized"]
    del tagdata["Image DateTime"]

    # combine "Resolution" values
    if ((tagdata["Image XResolution"] and
         tagdata["Image YResolution"] and
         tagdata["Image ResolutionUnit"])):
        x_res = tagdata["Image XResolution"]
        y_res = tagdata["Image YResolution"]
        if x_res == y_res:
            resolution = x_res
        else:
            resolution = x_res + "x" + y_res
        unit = tagdata["Image ResolutionUnit"]
        unit = unit.replace("Pixels/Inch", "ppi").replace("Dots/Inch", "dpi")
        tagdata["Resolution"] = "{resolution} {unit}".format(
            resolution=resolution, unit=unit)
    del tagdata["Image XResolution"]
    del tagdata["Image YResolution"]
    del tagdata["Image ResolutionUnit"]

    # combine "FocalLength" values
    if ((tagdata["EXIF FocalLength"] and
         tagdata["EXIF FocalLengthIn35mmFilm"])):
        focal_length = "{focal_35mm} (35mm film), {focal_lense} (lens)"
        tagdata["EXIF FocalLength"] = _(focal_length).format(
            focal_35mm=tagdata["EXIF FocalLengthIn35mmFilm"],
            focal_lense=tagdata["EXIF FocalLength"])
    del tagdata["EXIF FocalLengthIn35mmFilm"]

    # add "pixels" to width and height
    # and store them in Video's WIDTH/HEIGHT in order to reduce columns
    if ((tagdata.get("EXIF ExifImageWidth", None) and
         tagdata.get("EXIF ExifImageLength", None))):
        tagdata["VIDEO WIDTH"] = (_("%s pixels") %
                                  tagdata["EXIF ExifImageWidth"])
        tagdata["VIDEO HEIGHT"] = (_("%s pixels") %
                                   tagdata["EXIF ExifImageLength"])
        del tagdata["EXIF ExifImageWidth"]
        del tagdata["EXIF ExifImageLength"]

    # calculate f-number
    if tagdata["EXIF FNumber"]:
        first, x, second = tagdata["EXIF FNumber"].partition("/")
        fnumber = float(first) / float(second)
        tagdata["EXIF FNumber"] = "f/" + str(fnumber)

    # translate some words
    translatable_tags = (
        "EXIF Flash", "EXIF Orientation", "EXIF ExposureProgram",
        "EXIF MeteringMode", "ExposureMode", "EXIF WhiteBalance",
        "EXIF Contrast", "EXIF Saturation", "EXIF Sharpness"
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

    tagdata = dict()
    tagnames = set(TAGS_IMAGE_STUB.keys()).union(TAGS_IMAGE_STUB_HIDDEN)

    for tagname in tagnames:
        pixbuf_option = "tEXt::" + tagname
        # store some tags in other columns in order to reduce their amount
        if tagname == "Title":
            tagname = "GENERAL TRACK NAME"
        elif tagname == "Author":
            tagname = "AUTHOR"
        elif tagname == "Creation Time":
            tagname = "GENERAL RECORDED DATE"
        elif tagname == "Software":
            tagname = "Image Software"
        tagdata[tagname] = pixbuf.get_option(pixbuf_option)

    tagdata["VIDEO WIDTH"] = _("%s pixels") % str(pixbuf.get_width())
    tagdata["VIDEO HEIGHT"] = _("%s pixels") % str(pixbuf.get_height())

    return tagdata


def read_pdf_data(filename):
    """Read and return PDF metadata defined in TAGS_PDF.

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

    tagdata = dict()
    for line in pdfinfo.split("\n"):
        tagname, _, tagvalue = line.partition(":")
        tagname = tagname.upper()
        tagvalue = tagvalue.strip()
        if not tagvalue:
            continue

        if ((tagname in TAGS_PDF or
             tagname in TAGS_PDF_HIDDEN)):
            # store "title" tag in Audio's title tag in order to reduce columns
            if tagname == "TITLE":
                tagname = "GENERAL TRACK NAME"

            if tagname.endswith("DATE"):
                tagvalue = translate_poppler_date(tagvalue)

            tagdata[tagname] = tagvalue

    return tagdata


def translate_poppler_date(english_date):
    """Translate "english date" into locale date representation.

    ("english date" means the date format used by Poppler's pdfinfo.)
    """
    locale.setlocale(locale.LC_TIME, ("en", "utf-8"))
    time_object = time.strptime(english_date, "%a %b %d %H:%M:%S %Y")
    init_locale()
    locale_time = time.strftime("%c", time_object)
    return locale_time


def translate_exif_words(tagname, tagvalue):
    """Translate common EXIF words into user's locale.

    tagname -- the EXIF tag's name
    tagvalue -- the tag's value to be translated
    """
    # replace every part in Flash info with its translated version
    if tagname == "EXIF Flash":
        flash_parts = tagvalue.split(", ")
        tagvalue = ""
        for part in flash_parts:
            tagvalue += _(part) + ", "
        tagvalue = tagvalue.rstrip(", ")
    # all other tags can be replaced directly
    else:
        tagvalue = _(tagvalue)

    return tagvalue


def convert_duration(duration):
    """Convert textual duration into (sortable) colon representation.

    "3mn 8s" will be "03:08"
    "40s 512ms" will be "00:40"
    "2h 10mn" will be "h2:10"
    """
    colon_duration = ""
    for char in duration:
        # stop after seconds, that's precise enough
        if char == "s":
            break
        # replace spaces by colons
        if char.isspace():
            colon_duration += ":"
        # take only digits
        if char.isdigit():
            colon_duration += char
    # if we deal with hours, prepend an "h" to indicate that
    if "h" in duration:
        colon_duration = "h" + colon_duration
    # prepend "00:" if length < 1 min.
    if "mn" not in duration:
        colon_duration = "00:" + colon_duration

    # prepend zeros where necessary
    unified_duration = ""
    for part in colon_duration.split(":"):
        if len(part) < 2:
            unified_duration += ":0" + part
        else:
            unified_duration += ":" + part
    # remove accidentally added first colon
    unified_duration = unified_duration[1:]

    return unified_duration
