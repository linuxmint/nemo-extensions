#!/usr/bin/python3
"""nemo-git -- Nemo extension for git branch and file info.

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

import gettext
import locale
import os
import signal
import subprocess
from urllib.parse import unquote

from gi.repository import Nemo, Gtk, GObject


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)


TEXTDOMAIN = "nemo-git"


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
NAME_DESC = "Nemo Git:::" + _("Show git branch and file info in an "
                              "infobar and in columns")


# button response IDs (not used)
# RESPONSE_1 = 1
# RESPONSE_2 = 2
# string attribute name for the git file status
GIT_FILE_STATUS = "GitFileStatus"
# our git info script
PATH_GIT_INFO = "/usr/share/nemo-git/gitinfo.sh"


class NemoGit(GObject.GObject,
              Nemo.LocationWidgetProvider,
              Nemo.ColumnProvider,
              Nemo.InfoProvider,
              Nemo.NameAndDescProvider):
    """Provide branch and file info in git working directories.

    In such directories an infobar will be displayed showing the current
    working branch and a summary of the file status. Besides, a column
    adds the per-item status (Git file status, like "M" for modified).
    """

    def __init__(self):
        """Initialize empty mappings for ignored files and known status."""
        self.ignored = {}
        self.files_status = {}

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [NAME_DESC]

    def get_columns(self):
        """Return a NemoColumn for Git file status."""
        return Nemo.Column(name="NemoGit::FileStatus",
                           attribute=GIT_FILE_STATUS,
                           label="Git",
                           description=_("Git File Status")),

    def update_file_info(self, file_object):
        """Add the git file status to this file, if available.

        If the git binary did not provide a file status, use " " because
        the file should be up-to-date then.
        """
        if file_object.get_uri_scheme() != "file":
            return
        filename = unquote(file_object.get_uri()[7:])

        file_status = self.files_status.get(filename, " ")

        if not file_status:
            return
        file_object.add_string_attribute(GIT_FILE_STATUS, file_status)

        return Nemo.OperationResult.COMPLETE

    def get_widget(self, uri, window):
        """Return the InfoBar.

        Add the current Git Branch and a summary of changed/untracked
        files. Also, add a close button.
        """
        if not uri.startswith("file"):
            return
        path = uri[7:]
        # check if this (sub-)path is in the ignore list for this window
        for ignored in self.ignored.get(window, []):
            if path.startswith(ignored):
                return

        # query git binary
        git_output = get_git_output(path)
        if not git_output:
            return

        # extract info from output
        toplevel, status_on_branch = git_output.split("\n", 1)
        branch_line, status_lines = status_on_branch.split("\x00", 1)
        branch_label = make_branch_info(branch_line)
        status_label = make_status_info(status_lines, toplevel)
        # pass status lines to parser
        self.set_files_status(status_lines, toplevel)

        # create infobar
        infobar = Gtk.InfoBar(show_close_button=True)
        infobar.get_content_area().add(branch_label)
        infobar.connect("response", self.cb_on_infobar_button, toplevel, infobar, window)
        if status_label:
            infobar.get_content_area().add(status_label)
        infobar.show_all()
        return infobar

    def set_files_status(self, status_lines, toplevel):
        """Update the files status with {absolute file path : git status}.

        Read the git output from status_lines consisting of the file
        status and the file path relative to the git toplevel directory
        and prepend the path of that toplevel. Map the item's status to
        this absolute path.
        """
        files_status = {}
        for line in status_lines.split("\x00"):
            status, file_path = line[:2], line[3:]
            absolute_file_path = "/".join((toplevel, file_path))
            absolute_file_path = absolute_file_path.rstrip("/")
            files_status[absolute_file_path] = status
        self.files_status = files_status

    def cb_on_infobar_button(self, button, response_id, toplevel, infobar, window):
        """Callback for button clicks in the InfoBar.

        If Close is clicked, add this git project to the ignore list.
        """
        if response_id == int(Gtk.ResponseType.CLOSE):
            # add root working dir of this git project to ignore list
            ignored = self.ignored.setdefault(window, set())
            ignored.add(toplevel)
            self.ignored.update({window: ignored})
            infobar.destroy()
        else:
            # no other buttons
            pass


def get_git_output(path):
    """Invoke our git script which reads the branch info and files status.

    The file status is relative to this path so we can identify later
    whether the changes are in this directory.
    By making only one system call to the shell script, we avoid overhead.
    """
    try:
        os.chdir(path)
        cmd = (PATH_GIT_INFO)
        output = subprocess.check_output(cmd, stderr=subprocess.DEVNULL)
        return output.decode()
    except (subprocess.CalledProcessError, UnicodeError, EnvironmentError):
        return None


def make_branch_info(branch_line):
    """Return a GtkLabel with the local branch info.

    Remote info will be stripped.
    """
    remote_branch_index = branch_line.rfind("...")
    if remote_branch_index > -1:
        branch_name = branch_line[3:remote_branch_index]
    else:
        branch_name = branch_line[3:]

    branch_name = GObject.markup_escape_text(branch_name)
    branch_info = _("Git Branch: <i>%s</i>") % branch_name

    widget = Gtk.Label()
    widget.set_markup(branch_info)
    return widget


def make_status_info(status_lines, toplevel):
    """Return a GtkLabel with a summary of changed/untracked files.

    Also, a tooltip reveals the project's toplevel path and all changed/
    untracked files with their git status code.
    """
    status_lines = status_lines.rstrip("\x00")
    tooltip_text = _("Toplevel path") + ":\n   %s" % toplevel
    if not status_lines:
        status_info = _("Everything up-to-date.")
    else:
        status_array = status_lines.split("\x00")
        untracked = [status.lstrip("?? ") for status in status_array
                     if status.startswith("?")]
        changed = [status for status in status_array
                   if status.lstrip("?? ") not in untracked]
        num_untracked = len(untracked)
        num_changed = len(changed)
        status_info = ""

        if num_changed > 0:
            status_info = gettext.ngettext("%d change", "%d changes",
                                           num_changed) % num_changed
            tooltip_text += "\n\n" + _("Changes") + ":\n   " + "\n   ".join(changed)

        if num_untracked > 0:
            if num_changed:
                status_info += ", "
            status_info += gettext.ngettext("%d untracked", "%d untracked",
                                            num_untracked) % num_untracked
            tooltip_text += "\n\n" + _("Untracked") + ":\n   " + "\n   ".join(untracked)

    widget = Gtk.Label(status_info)
    widget.set_tooltip_text(tooltip_text)
    return widget
