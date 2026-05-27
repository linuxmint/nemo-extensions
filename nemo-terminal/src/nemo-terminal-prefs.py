#!/usr/bin/python3

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('XApp', '1.0')
import sys
import os
import shutil
import subprocess
import threading

import gettext
from gi.repository import Gtk, Gio, XApp, Gdk, GLib

# i18n
import gettext
gettext.bindtextdomain('nemo-extensions')
gettext.textdomain('nemo-extensions')
_ = gettext.gettext


class LabeledItem(Gtk.Box):
    def __init__(self, label, item):
        super(LabeledItem, self).__init__(orientation=Gtk.Orientation.HORIZONTAL)

        self.label_widget = Gtk.Label(label)

        self.pack_start(self.label_widget, False, False, 6)
        self.pack_end(item, False, False, 6)

        self.show_all()

class Page(Gtk.Box):
    def __init__(self):
        super(Page, self).__init__(orientation=Gtk.Orientation.VERTICAL)

        self.set_spacing(15)
        self.set_margin_start(15)
        self.set_margin_end(15)
        self.set_margin_top(15)
        self.set_margin_bottom(15)

class NemoTerminalPreferencesWindow(XApp.PreferencesWindow):
    def __init__(self):
        super(NemoTerminalPreferencesWindow, self).__init__()

        self.set_icon_name("terminal")
        self.set_title(_("Nemo-Terminal Preferences"))
        self.set_skip_taskbar_hint(False)
        self.set_type_hint(Gdk.WindowTypeHint.NORMAL)
        self.set_default_size(-1, -1)

        self.connect("destroy", Gtk.main_quit)

        self.settings = Gio.Settings(schema_id="org.nemo.extensions.nemo-terminal")
        self._theme_tool = self._resolve_theme_tool()

        # Basic

        page = Page()

        frame = Gtk.Frame()
        frame.get_style_context().add_class("view")

        page.add(frame)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        frame.add(box)

        switch = Gtk.Switch()
        self.settings.bind("default-visible",
                           switch, "active",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Visible by default"), switch)
        box.pack_start(widget, False, False, 6)

        combo = Gtk.ComboBoxText()
        combo.append("top", _("Top"))
        combo.append("bottom", _("Bottom"))
        self.settings.bind("terminal-position",
                           combo, "active-id",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Terminal position"), combo)
        box.pack_start(widget, False, False, 6)

        combo = Gtk.ComboBoxText()
        combo.append("None", _("Independent"))
        combo.append("Terminal follows Nemo", _("Terminal follows view location"))
        # combo.append("Nemo follows Terminal", _("View follows terminal location"))
        # combo.append("Nemo and Terminal Synchronized", _("Keep locations synchronized"))
        self.settings.bind("default-follow-mode",
                           combo, "active-id",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Location mode"), combo)
        box.pack_start(widget, False, False, 6)

        switch = Gtk.Switch()
        self.settings.bind("audible-bell",
                           switch, "active",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Terminal bell"), switch)
        box.pack_start(widget, False, False, 6)

        spinner = Gtk.SpinButton.new_with_range(5, 1000, 1)
        spinner.set_digits(0)
        self.settings.bind("default-terminal-height",
                           spinner, "value",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Default number of lines for the terminal"), spinner)
        box.pack_start(widget, False, False, 6)

        frame = Gtk.Frame()
        treeview = Gtk.TreeView(headers_visible=False, enable_search=False, hover_selection=True)
        cell = Gtk.CellRendererAccel(editable=True,
                                     accel_mode=Gtk.CellRendererAccelMode.GTK,
                                     width=140, xalign=0.5, yalign=0.5)

        col = Gtk.TreeViewColumn("binding", cell, accel_key=0, accel_mods=1)
        treeview.append_column(col)

        store = Gtk.ListStore(int, Gdk.ModifierType)
        treeview.set_model(store)

        def update_accel_from_settings(settings, data=None):
            accel_string = settings.get_string("terminal-hotkey")

            k, m = Gtk.accelerator_parse(accel_string)

            store.clear()
            store.append((k, m))

        self.settings.connect("changed::terminal-hotkey", update_accel_from_settings)
        update_accel_from_settings(self.settings)

        def on_accel_changed(accel, path, key, mods, code, data=None):
            name = Gtk.accelerator_name(key, mods)
            self.settings.set_string("terminal-hotkey", name)

            cell.set_property("text", Gtk.accelerator_get_label(key, mods))

        def on_accel_cleared(accel, path, data=None):
            self.settings.set_string("terminal-hotkey", "")

        cell.connect("accel-edited", on_accel_changed)
        cell.connect("accel-cleared", on_accel_cleared)

        frame.add(treeview)

        widget = LabeledItem(_("Keyboard shortcut"), frame)
        box.pack_start(widget, False, False, 6)

        self.add_page(page, "main", _("Basic"))

        # Appearance

        page = Page()

        frame = Gtk.Frame()
        frame.get_style_context().add_class("view")
        page.add(frame)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        frame.add(box)

        fontbutton = Gtk.FontButton()
        self.settings.bind("terminal-font",
                           fontbutton, "font",
                           Gio.SettingsBindFlags.DEFAULT)
        box.pack_start(LabeledItem(_("Font (blank for system monospace)"), fontbutton),
                       False, False, 6)

        box.pack_start(LabeledItem(_("Foreground color"),
                                   self._make_color_button("terminal-foreground-color")),
                       False, False, 6)
        box.pack_start(LabeledItem(_("Background color"),
                                   self._make_color_button("terminal-background-color")),
                       False, False, 6)
        box.pack_start(LabeledItem(_("Cursor color"),
                                   self._make_color_button("terminal-cursor-color")),
                       False, False, 6)

        combo = Gtk.ComboBoxText()
        combo.append("block", _("Block"))
        combo.append("ibeam", _("Beam"))
        combo.append("underline", _("Underline"))
        self.settings.bind("terminal-cursor-shape",
                           combo, "active-id",
                           Gio.SettingsBindFlags.DEFAULT)
        box.pack_start(LabeledItem(_("Cursor shape"), combo), False, False, 6)

        spinner = Gtk.SpinButton.new_with_range(-1, 1000000, 100)
        spinner.set_digits(0)
        self.settings.bind("terminal-scrollback-lines",
                           spinner, "value",
                           Gio.SettingsBindFlags.DEFAULT)
        box.pack_start(LabeledItem(_("Scrollback lines (-1 for unlimited)"), spinner),
                       False, False, 6)

        # Gogh theme picker

        frame = Gtk.Frame()
        frame.get_style_context().add_class("view")
        page.add(frame)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        frame.add(box)

        self._theme_combo = Gtk.ComboBoxText.new_with_entry()

        # Type-to-filter over the (long) theme list: a substring-matching
        # EntryCompletion on the combo's entry, so typing e.g. "mocha" narrows
        # the suggestions instead of scrolling hundreds of names.
        self._theme_store = Gtk.ListStore(str)
        completion = Gtk.EntryCompletion()
        completion.set_model(self._theme_store)
        completion.set_text_column(0)
        completion.set_match_func(self._theme_match_func, self._theme_store)
        completion.set_popup_completion(True)
        self._theme_combo.get_child().set_completion(completion)

        apply_button = Gtk.Button(_("Apply theme"))
        apply_button.set_valign(Gtk.Align.CENTER)
        apply_button.connect("clicked", self._on_apply_theme)

        theme_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        theme_row.pack_start(self._theme_combo, True, True, 0)
        theme_row.pack_end(apply_button, False, False, 6)
        box.pack_start(LabeledItem(_("Change theme (powered by Gogh)"), theme_row),
                       False, False, 6)

        self._theme_status = Gtk.Label(label="")
        self._theme_status.set_line_wrap(True)
        self._theme_status.set_xalign(0.0)
        box.pack_start(self._theme_status, False, False, 6)

        if self._theme_tool:
            threading.Thread(target=self._load_theme_list, daemon=True).start()
        else:
            self._theme_combo.set_sensitive(False)
            apply_button.set_sensitive(False)
            self._theme_status.set_text(
                _("'nemo-terminal-theme' not found — install it to switch themes."))

        self.add_page(page, "appearance", _("Appearance"))

        # Advanced

        page = Page()

        frame = Gtk.Frame()
        frame.get_style_context().add_class("view")

        page.add(frame)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        frame.add(box)

        entry = Gtk.Entry()
        self.settings.bind("terminal-shell",
                           entry, "text",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Shell to use (leave blank for system default)"), entry)
        box.pack_start(widget, False, False, 6)

        frame = Gtk.Frame()
        frame.get_style_context().add_class("view")

        page.add(frame)

        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        frame.add(box)

        entry = Gtk.Entry()
        self.settings.bind("terminal-erase-line",
                           entry, "text",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Terminal erase line key sequence"), entry)
        box.pack_start(widget, False, False, 6)

        entry = Gtk.Entry()
        self.settings.bind("terminal-restore-line",
                           entry, "text",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Terminal restore line key sequence"), entry)
        box.pack_start(widget, False, False, 6)

        entry = Gtk.Entry()
        self.settings.bind("terminal-change-directory-command",
                           entry, "text",
                           Gio.SettingsBindFlags.DEFAULT)

        widget = LabeledItem(_("Change directory command"), entry)
        box.pack_start(widget, False, False, 6)

        box.pack_start(Gtk.Separator(orientation=Gtk.Orientation.HORIZONTAL), False, False, 6)

        button = Gtk.Button(_("Restore defaults"))
        button.set_valign(Gtk.Align.CENTER)

        def on_reset_clicked(button, data=None):
            for key in self.settings.list_keys():
                self.settings.reset(key)

        button.connect("clicked", on_reset_clicked)

        widget = LabeledItem(_("Sequences must be escaped according to python rules.") + " " +
                             _("'%s' is replaced by the quoted directory name."), button)
        widget.label_widget.set_line_wrap(True)
        widget.label_widget.set_max_width_chars(40)
        widget.label_widget.set_justify(Gtk.Justification.FILL)

        box.pack_start(widget, False, False, 6)

        self.add_page(page, "advanced", _("Advanced"))

        self.show_all()

        self.present()

    def _make_color_button(self, key):
        """A ColorButton kept in sync with a string GSettings color key.

        ColorButton exposes a Gdk.RGBA property, not a string, so we bridge it
        by hand: load the key into the button, write the button's color back on
        change, and reload when the key changes elsewhere (e.g. a theme apply)."""
        button = Gtk.ColorButton()
        button.set_use_alpha(False)

        def load(*args):
            rgba = Gdk.RGBA()
            if rgba.parse(self.settings.get_string(key)):
                button.set_rgba(rgba)

        load()
        button.connect("color-set",
                       lambda b: self.settings.set_string(key, b.get_rgba().to_string()))
        self.settings.connect("changed::" + key, load)
        return button

    def _resolve_theme_tool(self):
        """Locate the nemo-terminal-theme importer (PATH, repo tools/, or /usr/bin)."""
        found = shutil.which("nemo-terminal-theme")
        if found:
            return found
        here = os.path.dirname(os.path.abspath(__file__))
        for candidate in (os.path.join(here, "..", "tools", "nemo-terminal-theme"),
                          "/usr/bin/nemo-terminal-theme"):
            if os.path.isfile(candidate) and os.access(candidate, os.X_OK):
                return os.path.abspath(candidate)
        return None

    def _load_theme_list(self):
        """Populate the theme dropdown from `nemo-terminal-theme --list` (off-thread)."""
        try:
            proc = subprocess.run([self._theme_tool, "--list"],
                                  capture_output=True, text=True, timeout=30)
            names = [n for n in proc.stdout.splitlines() if n.strip()] \
                if proc.returncode == 0 else []
        except Exception:
            names = []
        GLib.idle_add(self._populate_theme_combo, names)

    def _populate_theme_combo(self, names):
        for name in names:
            self._theme_combo.append_text(name)
            self._theme_store.append([name])
        return False

    def _theme_match_func(self, completion, key, iter_, store):
        # Match the typed text anywhere in the theme name (case-insensitive).
        # GTK passes `key` already case-folded.
        return key in store[iter_][0].lower()

    def _on_apply_theme(self, button):
        name = (self._theme_combo.get_active_text() or "").strip()
        if not name:
            self._theme_status.set_text(_("Enter or pick a theme name first."))
            return
        self._theme_status.set_text(_("Applying '%s'…") % name)
        threading.Thread(target=self._apply_theme_thread,
                         args=(name,), daemon=True).start()

    def _apply_theme_thread(self, name):
        try:
            proc = subprocess.run([self._theme_tool, name],
                                  capture_output=True, text=True, timeout=60)
            ok = proc.returncode == 0
            output = (proc.stdout if ok else proc.stderr).strip().splitlines()
            msg = output[-1] if output else (_("done") if ok else _("failed"))
        except Exception as exc:
            ok, msg = False, str(exc)
        GLib.idle_add(self._theme_status.set_text, ("✓ " if ok else "✗ ") + msg)

    def quit(self, *args):
        self.destroy()
        Gtk.main_quit()

if __name__ == "__main__":
    import signal
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    window = NemoTerminalPreferencesWindow()

    Gtk.main()
