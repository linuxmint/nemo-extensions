#!/usr/bin/python2
# -*- coding: UTF-8 -*-

############################################################################
##                                                                        ##
## Nemo Terminal - A terminal embedded in Nemo                            ##
##                                                                        ##
## Nemo Terminal is based heavily on Nautilus Terminal:                   ##
## Copyright (C) 2011  Fabien LOISON <flo at flogisoft dot com>           ##
## (http://projects.flogisoft.com/nautilus-terminal/)                     ##
##                                                                        ##
## This program is free software: you can redistribute it and/or modify   ##
## it under the terms of the GNU General Public License as published by   ##
## the Free Software Foundation, either version 3 of the License, or      ##
## (at your option) any later version.                                    ##
##                                                                        ##
## This program is distributed in the hope that it will be useful,        ##
## but WITHOUT ANY WARRANTY; without even the implied warranty of         ##
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          ##
## GNU General Public License for more details.                           ##
##                                                                        ##
## You should have received a copy of the GNU General Public License      ##
## along with this program.  If not, see <http://www.gnu.org/licenses/>.  ##
##                                                                        ##
## Nemo Terminal : http://github.com/linuxmint/nemo-extensions/           ##
## Nautilus Terminal: http://projects.flogisoft.com/Nemo-terminal/        ##
##                                                                        ##
############################################################################


"""A terminal embedded in Nemo."""

__author__ = "Fabien LOISON <flo at flogisoft dot com>"
__maintainer__ = "Will Rouesnel <w.rouesnel@gmail.com>"
__version__ = "1.0"
__appname__ = "nemo-terminal"
__app_disp_name__ = "Nemo Terminal"
__website__ = "http://github.com/linuxmint/nemo-extensions"


import os
import sys
from signal import SIGTERM, SIGKILL
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

import gettext
gettext.bindtextdomain('nemo-terminal', '@prefix@/share/locale')
gettext.textdomain('nemo-terminal')
_ = gettext.gettext

import gi
gi.require_version('Vte', '2.91')
gi.require_version('Nemo', '3.0')
from gi.repository import GObject, Nemo, Gtk, Gdk, Vte, GLib, Gio

# DEFAULT_CONF = {
#         'general/def_term_height': 5, #lines
#         'general/def_visible': False,
#         'general/term_on_top': True,
#         'terminal/shell': Vte.get_user_shell(),
#         }

BASE_KEY = "org.nemo.extensions.nemo-terminal"
settings = Gio.Settings.new(BASE_KEY)

def terminal_or_default():
    """Enforce a default value for terminal from GSettings"""
    terminalcmd = settings.get_string("terminal-shell")
    if (terminalcmd == "") or (terminalcmd is None):
        terminalcmd = Vte.get_user_shell()
    return terminalcmd

class NemoTerminal(object):
    """Nemo Terminal itself.

    Args:
        uri -- The URI of the folder where the terminal will be created.
        window -- The parent window.
    """

    def __init__(self, uri, window):
        """The constructor."""
        self._window = window
        self._path = self._uri_to_path(uri)
        #Term
        self.shell_pid = -1
        self.term = Vte.Terminal()

        settings.bind("audible-bell", self.term, "audible-bell", Gio.SettingsBindFlags.GET)

        self.shell_pid = self.term.spawn_sync(Vte.PtyFlags.DEFAULT, self._path, [terminal_or_default()], None, GLib.SpawnFlags.SEARCH_PATH, None, None, None)[1]

        # Make vte.sh active
        #vte_current_dir_script = ". /etc/profile.d/vte.sh ; clear"
        #self.term.feed_child(vte_current_dir_script, len(vte_current_dir_script))

        self.term.connect_after("child-exited", self._on_term_child_exited)
        self.term.connect_after("popup-menu", self._on_term_popup_menu)
        self.term.connect("button-press-event", self._on_term_popup_menu)

        #Accelerators
        accel_group = Gtk.AccelGroup()
        self._window.add_accel_group(accel_group)
        self.term.add_accelerator(
            "paste-clipboard",
            accel_group,
            ord("V"),
            Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK,
            Gtk.AccelFlags.VISIBLE)
        self.term.add_accelerator(
            "copy-clipboard",
            accel_group,
            ord("C"),
            Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK,
            Gtk.AccelFlags.VISIBLE)
        #Drag & Drop
        self.term.drag_dest_set(
            Gtk.DestDefaults.MOTION |
            Gtk.DestDefaults.HIGHLIGHT |
            Gtk.DestDefaults.DROP,
            [Gtk.TargetEntry.new("text/uri-list", 0, 80)],
            Gdk.DragAction.COPY,
        )
        self.term.drag_dest_add_uri_targets()
        self.term.connect("drag_data_received", self._on_drag_data_received)

        # Container
        self.vscrollbar = Gtk.VScrollbar.new(self.term.get_vadjustment())

        self.hbox = Gtk.HBox()
        self.hbox.pack_start(self.term, True, True, 0)
        self.hbox.pack_end(self.vscrollbar, False, False, 0)

        self.hbox.nt = self # store a reference to this obj

        #Popup Menu
        self.menu = Gtk.Menu()
        #MenuItem => copy
        menu_item = Gtk.ImageMenuItem.new_from_stock("gtk-copy", None)
        menu_item.connect_after("activate",
                                lambda w: self.term.copy_clipboard())
        self.menu.add(menu_item)
        #MenuItem => paste
        menu_item = Gtk.ImageMenuItem.new_from_stock("gtk-paste", None)
        menu_item.connect_after("activate",
                                lambda w: self.term.paste_clipboard())
        self.menu.add(menu_item)
        #MenuItem => paste filenames (only added if URI types on clipboard)
        menu_item_pastefilenames = \
            Gtk.ImageMenuItem.new_from_stock("gtk-paste", None)
        menu_item_pastefilenames.connect_after("activate",
                                               lambda w: self._paste_filenames_clipboard())
        menu_item_pastefilenames.set_label(_("Paste Filenames"))
        self.menu_item_pastefilenames = menu_item_pastefilenames
        self.menu.add(menu_item_pastefilenames)
        #MenuItem => separator #TODO: Implement the preferences window
        #menu_item = Gtk.SeparatorMenuItem()
        #self.menu.add(menu_item)
        #MenuItem => preferences
        #menu_item = Gtk.ImageMenuItem.new_from_stock("gtk-preferences", None)
        #self.menu.add(menu_item)
        #MenuItem => separator
        #menu_item = Gtk.SeparatorMenuItem()
        #self.menu.add(menu_item)
        #MenuItem => Goto current terminal directory
        #menu_item = Gtk.MenuItem.new_with_label(_("Goto current terminal directory"))
        #menu_item.connect_after("activate",
        #        lambda w: self._goto_current_terminal_directory())
        #self.menu.add(menu_item)
        #MenuItem => separator
        menu_item = Gtk.SeparatorMenuItem()
        self.menu.add(menu_item)
        #MenuItem => About
        menu_item = Gtk.ImageMenuItem.new_from_stock("gtk-about", None)
        menu_item.connect_after("activate",
                                lambda w: self.show_about_dialog())
        self.menu.add(menu_item)
        self.menu.show_all()

        self._set_term_height(settings.get_int("default-terminal-height"))
        self._visible = True

        #Lock
        self._respawn_lock = False
        #Register the callback for show/hide
        if hasattr(window, "toggle_hide_cb"):
            window.toggle_hide_cb.append(self.set_visible)

    def _goto_current_terminal_directory(self):
        """Navigate the active Nemo pane to the current working directory
        of the VTE

        This functionality depends on some changes to Nemo's extension
        interface.
        """
        # Well behaved shells will be updating VTE for us but it doesn't always
        # work.
        workingDir = self.term.get_current_directory_uri()
        if workingDir is None:
            # Find the current foreground process in terminal
            pty = self.term.get_pty_object()
            ptyfd = pty.get_fd()
            pgroup = os.tcgetpgrp(ptyfd)
            workingDir = os.readlink('/proc/%s/cwd' % pgroup)
        gfile = Gio.file_parse_name(workingDir)
        workingDirUri = gfile.get_uri()
        print workingDirUri
        # TODO: something useful (like changing the working dir)
        return

    def _paste_filenames_clipboard(self):
        """Paste to the VTE clipboard, converting URIs to literal filenames
        first.
        """
        gtkClipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
        if gtkClipboard.wait_is_uris_available():
            uris = gtkClipboard.wait_for_uris()
            concatfilenames = ""
            for idx, uri in enumerate(uris):
                path = self._uri_to_path(uri)
                if path == "":
                    continue
                quoted = GLib.shell_quote(path)
                self.feed_child(quoted)
                if idx != (len(uris)-1):
                    self.feed_child(' ')
        return

    def change_directory(self, uri):
        """Change the current directory in the shell if it is not busy.

        Args:
            uri -- The URI of the destination directory.
        """

        if settings.get_enum("default-follow-mode") in (0, 2):
            return 

        self._path = self._uri_to_path(uri)

        if self._path == "":
            return

        if not self._shell_is_busy():
            # Clear any input
            eraselinekeys = settings.get_string("terminal-erase-line").decode('string_escape')
            self.feed_child(eraselinekeys)

            # Change directory
            cdcmd_nonewline = settings.get_string("terminal-change-directory-command") \
                % GLib.shell_quote(self._path)
            cdcmd = "%s\n" % cdcmd_nonewline
            self.feed_child(cdcmd)

            # Restore user input
            restorelinekeys = settings.get_string("terminal-restore-line").decode('string_escape')
            self.feed_child(restorelinekeys)

    def get_widget(self):
        """Return the top-level widget of Nemo Terminal."""
        #if not self.term.get_parent():
        #    self.hbox.add(self.term)
        if self._visible:
            self.hbox.show_all()
        return self.hbox

#     def resize_term(self):
#         """Correctly update the size of VTE terminal view"""
#         print "resize term"
#         height = self.swin.get_allocated_height()
#         width = self.swin.get_allocated_width()
#
#         # bail if nothing to do
#         if self.term.get_allocated_height() == height and \
#             self.term.get_allocated_width() == width:
#             return
#
#         char_width = self.term.get_char_width()
#         char_height = self.term.get_char_height()
#
#         inner_border = self.term.get_style().get_property("inner-border")
#         new_grid_width = width - (inner_border.left + inner_border.right) / char_width
#         new_grid_height = height - (inner_border.top + inner_border.bottom) / char_height
#
#         grid_width = new_grid_width if new_grid_width > 0 else 0
#         grid_height = new_grid_height if new_grid_height > 0 else 0
#
#         self.term.set_size(grid_width, grid_height)

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
        about_dlg = Gtk.AboutDialog()
        #Set the content of the dialog
        about_dlg.set_program_name(__app_disp_name__)
        about_dlg.set_version(__version__)
        about_dlg.set_comments(__doc__)
        about_dlg.set_website(__website__)
        about_dlg.set_copyright("Nemo Terminal is based heavily on Nautilus Terminal by %s Copyright (c) 2011" % __author__)
        about_dlg.set_authors([__author__, __maintainer__])
        logo = Gtk.Image.new_from_file(
            "/usr/share/nemo-terminal/logo_120x120.png")
        about_dlg.set_logo(logo.get_pixbuf())
        #Signal
        about_dlg.connect("response", lambda w, r: w.destroy())
        #Display de dialog
        about_dlg.show()

    def destroy(self):
        """Release widgets and the shell process."""
        #Terminate the shell
        self._respawn_lock = True
        try:
            os.kill(self.shell_pid, SIGTERM)
            os.kill(self.shell_pid, SIGKILL)
        except OSError:
            pass
        #Remove some widgets
        self.vscrollbar.destroy()
        self.term.destroy()
        self.hbox.destroy()
        #Remove callback
        if hasattr(self._window, "toggle_hide_cb"):
            self._window.toggle_hide_cb.remove(self.set_visible)

    def _shell_is_busy(self):
        """Check if the shell is waiting for a command or not."""

        pty = self.term.get_pty()
        fd = pty.get_fd()

        fgpid = os.tcgetpgrp(fd)

        if fgpid == -1 or fgpid == self.shell_pid:
            return False

        return True

    def _uri_to_path(self, uri):
        """Returns the path corresponding of the given URI.

        Args:
            uri -- The URI to convert."""
        gfile = Gio.file_parse_name(uri)

        path = gfile.get_path()
        if path:
            return path
        else:
            return ""

    def _set_term_height(self, height):
        """Change the terminal height.

        Args:
            height -- The new height (in lines).
        """
        self.hbox.set_size_request(-1,
                                   height * self.term.get_char_height() + 2)

    def _on_term_popup_menu(self, widget, event=None):
        """Displays the contextual menu on right-click and menu-key."""
        if event and event.button != 3:
            return Gdk.EVENT_PROPAGATE
        # Should the Paste Filenames option be displayed?
        gtkClipboard = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
        if gtkClipboard.wait_is_uris_available():
            self.menu_item_pastefilenames.show()
        else:
            self.menu_item_pastefilenames.hide()
        self.menu.popup_at_pointer(event)

        return Gdk.EVENT_STOP

    def _on_term_child_exited(self, term, status):
        """Called when the shell is terminated.

        Args:
            term -- The VTE terminal (self.term).
        """
        if not self._respawn_lock:
            if self._path == "":
                self._path = GLib.get_home_dir()
            self.shell_pid = self.term.spawn_sync(Vte.PtyFlags.DEFAULT, self._path, [terminal_or_default()], None, GLib.SpawnFlags.SEARCH_PATH, None, None, None)[1]

    def _on_drag_data_received(self, widget, drag_context, x, y, data, info, time):
        """Handles drag & drop."""
        for uri in data.get_uris():
            if self._uri_to_path(uri) == "":
                continue

            path = "'%s' " % self._uri_to_path(uri).replace("'", r"'\''")
            self.feed_child(path)

    def feed_child(self, text):
        """
        gobject-introspection/python-gi differences force us to try with and
        without the text length. One of them will work.
        """
        try:
            self.term.feed_child(text)
        except TypeError:
            self.term.feed_child(text, len(text))

class Crowbar(object):
    """Modify the Nemo' widget tree when the crowbar is inserted in it.

    Args:
        uri -- The URI of the current directory.
        window -- The Nemo' window.
    """

    def __init__(self, uri, window):
        """The constructor."""
        self._uri = uri
        self._window = window
        #Crowbar
        self._crowbar = Gtk.EventBox()
        self._crowbar.connect_after("parent-set", self._on_crowbar_parent_set)
        #Lock
        self._lock = False

    def get_widget(self):
        """Returns the crowbar."""
        return self._crowbar

    def _on_crowbar_parent_set(self, widget, old_parent):
        """Called when the crowbar is inserted in the Nemo' widget tree.

        Args:
            widget -- The crowbar (self._crowbar).
            old_parent -- The previous parent of the crowbar (None...).
        """
        #Check if the work has already started
        if self._lock:
            return
        else:
            self._lock = True
        #Get the parents of the crowbar
        crowbar_p = self._crowbar.get_parent()
        crowbar_pp = crowbar_p.get_parent()
        crowbar_ppp = crowbar_pp.get_parent()
        crowbar_pp.connect_after("parent-set", self._on_crowbar_pp_parent_set)
        #Get the children of crowbar_pp
        crowbar_pp_children = crowbar_pp.get_children()
        #Check if our vpan is already there
        if type(crowbar_ppp) == Gtk.VPaned:
            #Find the Nemo Terminal
            nterm = None
            for crowbar_ppp_child in crowbar_ppp.get_children():
                if type(crowbar_ppp_child) == Gtk.HBox:
                    if hasattr(crowbar_ppp_child, "nt"):
                        nterm = crowbar_ppp_child.nt
                    break
            #Update the terminal (cd,...)
            if nterm:
                nterm.change_directory(self._uri)
        #New tab/window/split
        else:
            #Create the vpan
            vpan = Gtk.VPaned()
            vpan.show()
            vbox = Gtk.VBox()
            vbox.show()
            if settings.get_enum("terminal-position") == 0:
                vpan.add2(vbox)
            else:
                vpan.add1(vbox)
            #Add the vpan in Nemo, and reparent some widgets
            if len(crowbar_pp_children) == 2:
                for crowbar_pp_child in crowbar_pp_children:
                    crowbar_pp.remove(crowbar_pp_child)
                crowbar_pp.pack_start(vpan, True, True, 0)
                vbox.pack_start(crowbar_pp_children[0], False, False, 0)
                vbox.pack_start(crowbar_pp_children[1], True, True, 0)
            #Create the terminal
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
            if not vpan:
                print("[%s] W: Can't find the VPaned..." % __app_disp_name__)
                return
            hbox = None
            for child in vpan.get_children():
                if type(child) == Gtk.HBox:
                    hbox = child
            if not hbox:
                print("[%s] W: Can't find the GtkHBox..."
                      % __app_disp_name__)
                return
            if not hasattr(hbox, "nt"):
                print("[%s] W: Can't find the Nemo Terminal instance..."
                      % __app_disp_name__)
            hbox.nt.destroy()

class NemoTerminalProvider(GObject.GObject, Nemo.LocationWidgetProvider, Nemo.NameAndDescProvider):
    """Provides Nemo Terminal in Nemo."""

    def __init__(self):
        """The constructor."""
        print("[%s] I: Initializing the Nemo extension"
              % __app_disp_name__)

    def get_widget(self, uri, window):
        """Returns a "crowbar" that will add a terminal in Nemo.

        Args:
            uri -- The URI of the current directory.
            window -- The Nemo' window.
        """
        if not hasattr(window, "toggle_hide_cb"):
            window.toggle_hide_cb = []
        if not hasattr(window, "term_visible"):
            window.term_visible = settings.get_boolean("default-visible")
        # don't add terminals to the desktop
        if uri.startswith("x-nemo-desktop:///"):
            return
        #Event
        window.connect_after("key-press-event", self._toggle_visible)
        #Return the crowbar
        return Crowbar(uri, window).get_widget()

    def _toggle_visible(self, window, event):
        """Toggle the visibility of Nemo Terminal.

        This method is called on a "key-press-event" on the Nemo'
        window.

        Args:
            window -- The Nemo' window.
            event -- The detail of the event.
        """
        if event.keyval == \
                Gdk.keyval_from_name(settings.get_string("terminal-hotkey")):
            window.term_visible = not window.term_visible
            settings.set_boolean("default-visible",  window.term_visible)
            for callback in window.toggle_hide_cb:
                callback(window.term_visible)
            return Gdk.EVENT_STOP #Stop the event propagation

        return Gdk.EVENT_PROPAGATE

    def get_name_and_desc(self):
        return [("Nemo Terminal:::Embedded terminal for Nemo:::nemo-terminal-prefs")]

if __name__ == "__main__":
    #Code for testing Nemo Terminal outside of Nemo
    print("%s %s\nOriginally By %s\nPorted for Nemo by: %s"
          % (__app_disp_name__, __version__, __author__, __maintainer__))
    win = Gtk.Window()
    win.set_title("%s %s" % (__app_disp_name__, __version__))
    nterm = NemoTerminal("file://%s" % os.environ["HOME"], win)
    nterm._respawn_lock = True
    nterm.term.connect("child-exited", Gtk.main_quit)
    nterm.get_widget().set_size_request(nterm.term.get_char_width() * 80 + 2,
                                        nterm.term.get_char_height() * 24 + 2)
    win.connect_after("destroy", Gtk.main_quit)
    win.add(nterm.get_widget())
    win.show_all()
    Gtk.main()
