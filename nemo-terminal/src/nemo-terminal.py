#!/usr/bin/python3
"""nemo-terminal -- Nemo extension for embedding a terminal into it.

Copyright (C) 2011  Fabien Loison <flo at flogisoft dot com>
Copyright (C) 2013  Will Rouesnel <w.rouesnel@gmail.com>
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

import gi
from gi.repository import GObject, Nemo, Gtk, Gdk, Vte, GLib, Gio


__author__ = "Fabien Loison <flo at flogisoft dot com>"
__maintainer__ = "Will Rouesnel <w.rouesnel@gmail.com>"
__maintainer2__ = "Eduard Dopler <kontakt@eduard-dopler.de>"
__version__ = "3.0.0"
__appname__ = "nemo-terminal"
__app_disp_name__ = "Nemo Terminal"
__website__ = "http://github.com/linuxmint/nemo-extensions"
copyright_note = "Nemo Terminal is based heavily on Nautilus Terminal " \
                 "by %s Copyright (c) 2011" % __author__


# react on default SIGINT
signal.signal(signal.SIGINT, signal.SIG_DFL)

# in order to stay independent from API changes
gi.require_version("Vte", "2.90")


TEXTDOMAIN = "nemo-terminal"


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
NAME_DESC = "Nemo Terminal:::" + _("Add an embedded terminal to Nemo")


BASE_KEY = "org.nemo.extensions.nemo-terminal"
settings = Gio.Settings.new(BASE_KEY)


class NemoTerminal():

    """Embedded Terminal in Nemo. Main class."""

    def __init__(self, uri, window):
        """Initialize terminal, connect event signals.

        Args:
        uri -- The URI of the folder where the terminal will be created.
        window -- The parent window.
        """
        self._window = window
        self._path = self._uri_to_path(uri)
        # term
        self.shell_pid = -1
        self.term = Vte.Terminal()

        self.shell_pid = self.term.fork_command_full(
            Vte.PtyFlags.DEFAULT,
            self._path, [get_terminal_cmd()], None,
            GLib.SpawnFlags.SEARCH_PATH, None, None)[1]
        # connect events
        self.term.connect_after("child-exited", self._on_term_child_exited)
        self.term.connect_after("popup-menu", self._on_term_popup_menu)
        self.term.connect("button-release-event", self._on_term_popup_menu)

        # accelerators
        accel_group = Gtk.AccelGroup()
        self._window.add_accel_group(accel_group)
        modifier = Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK
        self.term.add_accelerator(
                "paste-clipboard",
                accel_group,
                ord("V"),
                modifier,
                Gtk.AccelFlags.VISIBLE)
        self.term.add_accelerator(
                "copy-clipboard",
                accel_group,
                ord("C"),
                modifier,
                Gtk.AccelFlags.VISIBLE)
        # drag & drop
        self.term.drag_dest_set(
                Gtk.DestDefaults.MOTION |
                Gtk.DestDefaults.HIGHLIGHT |
                Gtk.DestDefaults.DROP,
                [Gtk.TargetEntry.new("text/uri-list", 0, 80)],
                Gdk.DragAction.COPY,
                )
        self.term.drag_dest_add_uri_targets()
        self.term.connect("drag_data_received", self._on_drag_data_received)

        # container
        self.vscrollbar = Gtk.VScrollbar.new(self.term.adjustment)

        self.hbox = Gtk.HBox()
        self.hbox.pack_start(self.term, True, True, 0)
        self.hbox.pack_end(self.vscrollbar, False, False, 0)

        self.hbox.nt = self  # store a reference to this obj

        # context menu
        # copy
        self.menu = Gtk.Menu()
        menu_item = Gtk.ImageMenuItem.new_from_stock("gtk-copy", None)
        menu_item.connect_after("activate",
                                lambda w: self.term.copy_clipboard())
        self.menu.add(menu_item)
        # paste
        menu_item = Gtk.ImageMenuItem.new_from_stock("gtk-paste", None)
        menu_item.connect_after("activate",
                                lambda w: self.term.paste_clipboard())
        self.menu.add(menu_item)
        # paste paths (only active if URIs in clipboard)
        menu_item_pastefilenames = Gtk.ImageMenuItem.new_from_stock(
            "gtk-paste", None)
        menu_item_pastefilenames.connect_after(
            "activate", lambda w: self._paste_filenames_clipboard())
        menu_item_pastefilenames.set_label(_("Paste Path(s)"))
        self.menu_item_pastefilenames = menu_item_pastefilenames
        self.menu.add(menu_item_pastefilenames)

        menu_item = Gtk.SeparatorMenuItem()
        self.menu.add(menu_item)

        # about
        menu_item = Gtk.ImageMenuItem.new_from_stock("gtk-about", None)
        menu_item.connect_after("activate",
                                lambda w: self.show_about_dialog())
        self.menu.add(menu_item)

        self.menu.show_all()

        # set height from settings
        settings_default_height = settings.get_value("default-terminal-height")
        self._set_term_height(settings_default_height.get_byte())
        self._visible = True
        # lock
        self._respawn_lock = False
        # register the callback for show/hide
        if hasattr(window, "toggle_hide_cb"):
            window.toggle_hide_cb.append(self.set_visible)

    def _paste_filenames_clipboard(self):
        """Convert URIs from clipboard to paths and paste into VTE."""
        clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
        if clipboard.wait_is_uris_available():
            uris = clipboard.wait_for_uris()
            for idx, uri in enumerate(uris):
                path = Gio.file_parse_name(uri).get_path()
                quoted = GLib.shell_quote(path)
                self.term.feed_child(quoted, len(quoted))
                if idx + 1 != len(uris):
                    self.term.feed_child(" ", 1)

    def change_directory(self, uri):
        """Change the current directory in the shell if it is not busy.

        Args:
            uri -- The URI of the destination directory.
        """
        self._path = self._uri_to_path(uri)
        if not self._shell_is_busy():
            # clear any input
            erase_line_keys = settings.get_string("terminal-erase-line")
            erase_line_keys = erase_line_keys.encode().decode("unicode_escape")
            self.term.feed_child(erase_line_keys, len(erase_line_keys))
            # change directory
            cd_cmd = settings.get_string("terminal-change-directory-command")
            cd_cmd_nonewline = cd_cmd % GLib.shell_quote(self._path)
            cd_cmd = "%s\n" % cd_cmd_nonewline
            self.term.feed_child(cd_cmd, len(cd_cmd))
            # restore user input
            restore_line_keys = settings.get_string("terminal-restore-line")
            restore_line_keys = restore_line_keys.encode().decode("unicode_escape")
            self.term.feed_child(restore_line_keys, len(restore_line_keys))

    def get_widget(self):
        """Return the top-level widget of Nemo Terminal."""
        if self._visible:
            self.hbox.show_all()
        return self.hbox

    def set_visible(self, visible):
        """Change the visibility of Nemo Terminal.

        Args:
            visible -- True for showing Nemo Terminal, False for hiding.
        """
        self._visible = visible
        if visible:
            self.hbox.show_all()
            self._window.set_focus(self.term)
        else:
            self.hbox.hide()

    def show_about_dialog(self):
        """Display the about dialog."""
        developers = [
            __author__,
            __maintainer__,
            __maintainer2__
        ]
        about_dlg = Gtk.AboutDialog(
            program_name=__app_disp_name__,
            version=__version__,
            logo_icon_name="terminal",
            website=__website__,
            authors=developers,
            copyright=copyright_note
        )
        about_dlg.connect("response", lambda w, r: w.destroy())
        about_dlg.show()

    def destroy(self):
        """Release widgets and the shell process."""
        # terminate the shell
        self._respawn_lock = True
        try:
            os.kill(self.shell_pid, signal.SIGTERM)
            os.kill(self.shell_pid, signal.SIGKILL)
        except OSError:
            pass
        # remove other widgets
        self.vscrollbar.destroy()
        self.term.destroy()
        self.hbox.destroy()
        # remove hotkey callback
        if hasattr(self._window, "toggle_hide_cb"):
            self._window.toggle_hide_cb.remove(self.set_visible)

    def _shell_is_busy(self):
        """Return whether the shell is waiting for a command."""
        wchan_path = "/proc/%i/wchan" % self.shell_pid
        wchan = open(wchan_path, "r").read()
        if wchan == "n_tty_read":
            return False
        elif wchan == "schedule":
            shell_stack_path = "/proc/%i/stack" % self.shell_pid
            try:
                for line in open(shell_stack_path, "r"):
                    if line.split(" ")[-1].startswith("n_tty_read"):
                        return False
                return True
            except IOError:
                return False
        elif wchan == "wait_woken":
            return False
        elif wchan == "do_wait":
            return True
        else:
            return True

    def _uri_to_path(self, uri):
        """Return path to corresponding URI."""
        return Gio.file_parse_name(uri).get_path()

    def _set_term_height(self, height):
        """Change the terminal height (in lines)."""
        new_height = height * self.term.get_char_height() + 2
        self.hbox.set_size_request(-1, new_height)

    def _on_term_popup_menu(self, widget, event=None):
        """Show context menu on right-clicks."""
        if ((not event or
             event.type != Gdk.EventType.BUTTON_RELEASE or
             event.button != 3)):
            return

        # show "Paste Paths" option in menu?
        clipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
        if clipboard.wait_is_uris_available():
            self.menu_item_pastefilenames.show()
        else:
            self.menu_item_pastefilenames.hide()

        self.menu.popup(None, None, None, None, 3, 0)

    def _on_term_child_exited(self, term):
        """Called when the shell is terminated."""
        if not self._respawn_lock:
            self.shell_pid = self.term.fork_command_full(
                Vte.PtyFlags.DEFAULT,
                self._path, [get_terminal_cmd()], None,
                GLib.SpawnFlags.SEARCH_PATH, None, None)[1]

    def _on_drag_data_received(self, widget, drag_context, x, y, data, info,
                               time):
        """Insert URIs received via drag and drop into VTE."""
        for uri in data.get_uris():
            path = "'%s' " % self._uri_to_path(uri).replace("'", r"'\''")
            self.term.feed_child(path, len(path))


class Crowbar():

    """Modify Nemo's widget tree when the crowbar is inserted in it.

    Args:
        uri -- The URI of the current directory.
        window -- The Nemo' window.
    """

    def __init__(self, uri, window):
        """Init."""
        self._uri = uri
        self._window = window
        # crowbar
        self._crowbar = Gtk.EventBox()
        self._crowbar.connect_after("parent-set", self._on_crowbar_parent_set)
        # lock
        self._lock = False

    def get_widget(self):
        """Return the crowbar."""
        return self._crowbar

    def _on_crowbar_parent_set(self, widget, old_parent):
        """Called when the crowbar is inserted in the Nemo' widget tree.

        Args:
            widget -- The crowbar (self._crowbar).
            old_parent -- The previous parent of the crowbar (None...).
        """
        # check if busy
        if self._lock:
            return
        else:
            self._lock = True
        # get the crowbar's parents
        crowbar_p = self._crowbar.get_parent()
        crowbar_pp = crowbar_p.get_parent()
        crowbar_ppp = crowbar_pp.get_parent()
        crowbar_pp.connect_after("parent-set", self._on_crowbar_pp_parent_set)
        # get the crowbar pp's children
        crowbar_pp_children = crowbar_pp.get_children()
        # check if the vpan is already created
        if type(crowbar_ppp) == Gtk.VPaned:
            # find the Nemo Terminal
            nterm = None
            for crowbar_ppp_child in crowbar_ppp.get_children():
                if type(crowbar_ppp_child) == Gtk.HBox:
                    if hasattr(crowbar_ppp_child, "nt"):
                        nterm = crowbar_ppp_child.nt
                    break
            # update the terminal (cd, ...)
            if nterm:
                nterm.change_directory(self._uri)
        # new tab/window/split
        else:
            # create the vpan
            vpan = Gtk.VPaned()
            vpan.show()
            vbox = Gtk.VBox()
            vbox.show()
            if settings.get_enum("terminal-position") == 0:
                vpan.add2(vbox)
            else:
                vpan.add1(vbox)
            # add the vpan in Nemo, and reparent some widgets
            if len(crowbar_pp_children) == 2:
                for crowbar_pp_child in crowbar_pp_children:
                    crowbar_pp.remove(crowbar_pp_child)
                crowbar_pp.pack_start(vpan, True, True, 0)
                vbox.pack_start(crowbar_pp_children[0], False, False, 0)
                vbox.pack_start(crowbar_pp_children[1], True, True, 0)
            # create the terminal
            nterm = NemoTerminal(self._uri, self._window)

            if hasattr(self._window, "term_visible"):
                nterm.set_visible(self._window.term_visible)
            if settings.get_enum("terminal-position") == 0:
                vpan.add1(nterm.get_widget())
            else:
                vpan.add2(nterm.get_widget())

    def _on_crowbar_pp_parent_set(self, widget, old_parent):
        """Called when the vpan parent lost his parent.

        Args:
            widget -- The vpan's parent.
            old_parent -- The previous parent.
        """
        if not widget.get_parent():
            vpan = None
            for child in widget.get_children():
                if type(child) == Gtk.VPaned:
                    vpan = child
                    break
            else:
                return
            hbox = None
            for child in vpan.get_children():
                if type(child) == Gtk.HBox:
                    hbox = child
                    break
            else:
                return
            if not hasattr(hbox, "nt"):
                return
            hbox.nt.destroy()


class NemoTerminalProvider(GObject.GObject,
                           Nemo.LocationWidgetProvider,
                           Nemo.NameAndDescProvider):

    """Provide the Nemo Terminal in Nemo."""

    def get_name_and_desc(self):
        """Return the name and a short description for the module overview."""
        return [NAME_DESC]

    def get_widget(self, uri, window):
        """Return a "crowbar" that will add a terminal in Nemo.

        Args:
            uri -- The URI of the current directory.
            window -- The Nemo window.
        """
        if not uri.startswith("file://"):
            return
        if not hasattr(window, "toggle_hide_cb"):
            window.toggle_hide_cb = []
        if not hasattr(window, "term_visible"):
            window.term_visible = settings.get_boolean("default-visible")
        # listen for toggle key events
        window.connect_after("key-press-event", self._toggle_visible)
        # return the crowbar
        return Crowbar(uri, window).get_widget()

    def _toggle_visible(self, window, event):
        """Toggle the visibility of Nemo Terminal."""
        terminal_hotkey = settings.get_string("terminal-hotkey")
        terminal_hotkey_keyval = Gdk.keyval_from_name(terminal_hotkey)
        if event.keyval == terminal_hotkey_keyval:
            window.term_visible = not window.term_visible
            for callback in window.toggle_hide_cb:
                callback(window.term_visible)
            return True  # stop the event propagation


def get_terminal_cmd():
    """Enforce a default value for terminal from GSettings."""
    terminal_cmd = settings.get_string("terminal-shell")
    if not terminal_cmd:
        terminal_cmd = Vte.get_user_shell()
    return terminal_cmd


# code for testing Nemo Terminal outside of Nemo
if __name__ == "__main__":
    print("%s %s\nOriginally By %s\nPorted for Nemo by: %s"
          % (__app_disp_name__, __version__, __author__, __maintainer__))
    win = Gtk.Window()
    win.set_title("%s %s" % (__app_disp_name__, __version__))
    nterm = NemoTerminal("file://" + os.environ["HOME"], win)
    nterm._respawn_lock = True
    nterm.term.connect("child-exited", Gtk.main_quit)
    nterm.get_widget().set_size_request(
        nterm.term.get_char_width() * 80 + 2,
        nterm.term.get_char_height() * 24 + 2)
    win.connect_after("destroy", Gtk.main_quit)
    win.add(nterm.get_widget())
    win.show_all()
    Gtk.main()
