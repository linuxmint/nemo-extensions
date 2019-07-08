# Nemo Hide - Extension for Nemo to hide files without renaming them
# Originally Adapted from Nautilus Hide Copyright (C) 2015 Bruno Nova <brunomb.nova@gmail.com>
# Forked from a discontinued project by "Emmanuel KDB2" on Github https://github.com/KDB2/Nemo-hide
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
from gi.repository import Nemo, GObject
from gettext import ngettext, locale, bindtextdomain, textdomain

class NemoHide(Nemo.MenuProvider, GObject.GObject):
	"""Simple Nemo extension that adds some actions to the context menu to
	hide and unhide files, by adding/removing their names to/from the folder's
	'.hidden' file."""
	def __init__(self):
		pass

	def get_file_items(self, window, files):
		"""Returns the menu items to display when one or more files/folders are
		selected."""
		# Make "files" paths relative and remove files that start with '.'
		# or that end with '~' (files that are already hidden)
		dir_path = None # path of the directory
		filenames = []
		for file in files:
			if dir_path == None: # first file: find path to directory
				dir_path = file.get_parent_location().get_path()
				if file.get_uri_scheme() != "file": # must be a local directory
					return
			name = file.get_name()
			if not name.startswith(".") and not name.endswith("~"):
				filenames += [name]

		if dir_path == None or len(filenames) == 0:
			return

		# Check if the user has write access to the ".hidden" file and its
		# directory
		hidden_path = dir_path + "/.hidden" # path to the ".hidden" file
		if not os.access(dir_path, os.W_OK | os.X_OK) or \
		   (os.path.exists(hidden_path) and not os.access(hidden_path, os.R_OK | os.W_OK)):
			return

		# Read the ".hidden" file
		try:
			hidden = set()
			if os.path.exists(hidden_path):
				with open(hidden_path) as f:
					for line in f.readlines():
						line = line.strip("\r\n") # strip newline characters
						if line != "":
							hidden.add(line)
		except: # ".hidden" file was deleted?
			hidden = set()

		# Determine what menu items to show (Hide, Unhide, or both)
		show_hide, show_unhide = False, False
		for file in filenames:
			if file in hidden:
				show_unhide = True
			else:
				show_hide = True
			if show_hide and show_unhide:
				break

		# Add the menu items
		items = []
		self._setup_gettext();
		if show_hide:
			items += [self._create_hide_item(filenames, hidden_path, hidden)]
		if show_unhide:
			items += [self._create_unhide_item(filenames, hidden_path, hidden)]

		return items


	def _setup_gettext(self):
		"""Initializes gettext to localize strings."""
		try: # prevent a possible exception
			locale.setlocale(locale.LC_ALL, "")
		except:
			pass
		bindtextdomain("Nemo-hide", "@CMAKE_INSTALL_PREFIX@/share/locale")
		textdomain("Nemo-hide")

	def _create_hide_item(self, files, hidden_path, hidden):
		"""Creates the 'Hide file(s)' menu item."""
		item = Nemo.MenuItem(name="NemoHide::HideFile",
		                         label=ngettext("_Hide File", "_Hide Files", len(files)),
		                         tip=ngettext("Hide this file", "Hide these files", len(files)))
		item.connect("activate", self._hide_run, files, hidden_path, hidden)
		return item

	def _create_unhide_item(self, files, hidden_path, hidden):
		"""Creates the 'Unhide file(s)' menu item."""
		item = Nemo.MenuItem(name="NemoHide::UnhideFile",
		                         label=ngettext("Un_hide File", "Un_hide Files", len(files)),
		                         tip=ngettext("Unhide this file", "Unhide these files", len(files)))
		item.connect("activate", self._unhide_run, files, hidden_path, hidden)
		return item

	def _update_hidden_file(self, hidden_path, hidden):
		"""Updates the '.hidden' file with the new filenames, or deletes it if
		empty (no files to hide)."""
		try:
			if hidden == set():
				if os.path.exists(hidden_path):
					os.remove(hidden_path)
			else:
				with open(hidden_path, "w") as f:
					for file in hidden:
						f.write(file + '\n')
						os.system("xte 'keydown Control_L' 'key R' 'keyup Control_L'")#refresh of Nemo
		except:
			print("Failed to delete or write to {}!".format(hidden_path))


	def _hide_run(self, menu, files, hidden_path, hidden):
		"""'Hide file(s)' menu item callback."""
		for file in files:
			hidden.add(file)
		self._update_hidden_file(hidden_path, hidden)

	def _unhide_run(self, menu, files, hidden_path, hidden):
		"""'Unhide file(s)' menu item callback."""
		for file in files:
			try:
				hidden.remove(file)
			except: # file not in "hidden"
				pass
		self._update_hidden_file(hidden_path, hidden)
