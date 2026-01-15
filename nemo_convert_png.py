'''Extension for Nemo's context menu to easily convert images to PNG and
optimize their filesize with pngcrush.'''
from __future__ import annotations

import os
import subprocess
from urllib.parse import unquote_plus, urlparse

from PIL import Image, UnidentifiedImageError
import PySimpleGUI as sg

import gi
gi.require_version('Nemo', '3.0')
from gi.repository import GObject, Nemo  # type: ignore pylint: disable=wrong-import-position


EXTENSIONS = ('jpg', 'jpeg', 'gif', 'tiff', 'bmp', 'png')
uqp= unquote_plus

def get_files(files_in: list[GObject]) -> list[str]|None:
    """
    Retrieve filenames as cross-platform safe strings from file objects.

    :param files_in: List of file objects.
    """
    files = []
    for file_in in files_in:
        file_in = unquote_plus(file_in.get_uri()[7:])
        if os.path.isfile(file_in):
            files.append(file_in)
    if files:
        return files
    return None

def convert_one(file: str) -> None:
    '''
    Converts an image to a PNG.

    :param file: Filename of the image to convert.
    '''
    filename = f'{file.split(".")[-2]}.png'
    try:
        img = Image.open(file).convert('RGB')
    except UnidentifiedImageError:
        img = False
    if img:
        os.remove(file)
        img.save(filename, 'PNG')


def convert_images(_, files: list[str]) -> list[str]:
    '''
    Called by the context menu item "Convert selected image(s) to PNG".

    :param files: The currently selected files.
    '''

    filenames = [f'{file.split(".")[-2]}.png' for file in files]
    count = sum(not file.endswith('png') for file in files)
    for i, file in enumerate(files):
        if not file.endswith('png'):
            sg.OneLineProgressMeter('Please wait...', i+1, count, 'pb', 'Converting images', orientation='h')
            convert_one(file)
    sg.OneLineProgressMeter('', count, count, key='pb')
    return filenames


def crush_one(file: str) -> None:
    '''
    Runs pngcrush on a png file.

    :param file: The file to execute this action on.
    '''
    subprocess.run(['pngcrush', '-rem', 'alla', '-nofilecheck', '-fix', '-ow',
                    '-reduce', '-m', '0', file], check=False)


def crush_images(_, files: list[str]) -> None:
    '''
    Called by the context menu item "Optimize image(s) with pngcrush.

    :param files: The currently selected files.
    '''
    for i, file in enumerate(files):
        sg.OneLineProgressMeter('Please wait...', i+1, len(files), 'pb',
                                'Optimize images with pngcrush', orientation='h')
        crush_one(file)
    sg.OneLineProgressMeter('', len(files), len(files), key='pb')



def convert_and_crush(_, files: list[str]) -> None:
    '''
    Called by the context menu item "Convert to PNG and optimize.

    :param files: The currently selected files.
    '''
    converted = convert_images(None, files)
    crush_images(None, converted)


class PNGConverter(GObject.GObject, Nemo.MenuProvider):
    '''Class for extension context menu items.'''

    def __init__(self):
        '''File manager crashes if init is not called.'''
        ...

    def get_background_items( # pylint: disable=arguments-differ
        self, _, folder: GObject) -> list[Nemo.MenuItem]|None:
        '''
        Called when context menu is called with no file objects selected.

        :param folder: Nemo's current working directory.
        '''

        folder = urlparse(folder.get_uri()).path
        files = [uqp(os.path.join(folder, f))
                 for f in os.listdir(uqp(folder))
                 if os.path.isfile(uqp(os.path.join(folder, f))) and
                 f.lower().endswith(EXTENSIONS)]

        if all(file.endswith('png') for file in files):
            crush = Nemo.MenuItem(
                name='CrushImages',
                label='Optimize image(s) with pngcrush',
                tip='Optimize image filesizes with pngcrush'
            )
            crush.connect('activate', crush_images, files)
            return [crush]

        if any(file.endswith(EXTENSIONS) for file in files):
            convert = Nemo.MenuItem(
                name="ConvertAllImagestoPNG",
                label="Convert all images to PNG",
                tip="Convert all images to PNG"
            )
            convert.connect('activate', convert_images, files)

            crush = Nemo.MenuItem(
                name='ConvertandCrush',
                label="Convert images to PNG and optimize",
                tip="Convert images to PNG and optimize filesizes with pngcrush"
            )
            crush.connect('activate', convert_and_crush, files)
            return [convert, crush]



    def get_file_items( # pylint: disable=arguments-differ
        self, _, files: list[GObject]) -> list[Nemo.MenuItem]|None:
        '''
        Called when context menu is called with files selected.

        :param files: The currently selected file objects.
        '''
        files = get_files(files) # type: ignore
        try:
            is_iter = iter(files)
            check = all(file.lower().endswith('png') for file in files)
        except TypeError:
            is_iter = False
            check = False
        if check:
            convert = Nemo.MenuItem(
                name="CrushImages",
                label="Optimize image(s) with pngcrush",
                tip="Optimize filesize(s) with pngcrush"
            )
            convert.connect('activate', crush_images, files)
            return [convert]

        if is_iter:
            check = all(file.lower().endswith(EXTENSIONS) for file in files)
        if check:
            convert = Nemo.MenuItem(
                name="ConvertImagetoPNG",
                label="Convert selected image(s) to .png",
                tip="Convert image(s) to .png"
            )
            convert.connect('activate', convert_images, files)

            crush = Nemo.MenuItem(
                name="ConvertandCrush",
                label="Convert to PNG and optimize with pngcrush",
                tip="Convert image(s) to PNG and optimize filesize(s) with\
                    pngcrush"
            )
            crush.connect('activate', convert_and_crush, files)
            return [convert, crush]
