#!/usr/bin/python3

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('XApp', '1.0')
import sys

import gettext
from gi.repository import Gtk, Gio, XApp, Gdk

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

    def quit(self, *args):
        self.destroy()
        Gtk.main_quit()

if __name__ == "__main__":
    import signal
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    window = NemoTerminalPreferencesWindow()

    Gtk.main()
