/*
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * The NemoPreview project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and NemoPreview. This
 * permission is above and beyond the permissions granted by the GPL license
 * NemoPreview is covered by.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

const Lang = imports.lang;

// util imports
const Path = imports.util.path;

// gi imports
const GLib = imports.gi.GLib;
const Gtk = imports.gi.Gtk;
const Gdk = imports.gi.Gdk;
const Gio = imports.gi.Gio;

const MainWindow = imports.ui.mainWindow;

const NEMO_PREVIEW_DBUS_PATH = '/org/nemo/Preview';
const NEMO_PREVIEW_DBUS_NAME = 'org.nemo.Preview';

const NemoPreviewIface = '<node> \
<interface name="org.nemo.Preview"> \
<method name="ShowFile"> \
    <arg type="s" direction="in" name="uri" /> \
    <arg type="i" direction="in" name="xid" /> \
    <arg type="b" direction="in" name="closeIfAlreadyShown" /> \
</method> \
<method name="Close"> \
</method> \
</interface> \
</node>';

const Application = new Lang.Class({
    Name: 'Application',
    Extends: Gtk.Application,

    _init : function(args) {
        this.parent({ application_id: NEMO_PREVIEW_DBUS_NAME });

        this._dbusImpl = Gio.DBusExportedObject.wrapJSObject(NemoPreviewIface, this);
        this._dbusImpl.export(Gio.DBus.session, NEMO_PREVIEW_DBUS_PATH);
    },

    vfunc_startup : function() {
        this.parent();

        this._defineStyleAndThemes();
        this._createMainWindow();
    },

    vfunc_activate : function() {
    },

    _createMainWindow : function() {
        this._mainWindow =
            new MainWindow.MainWindow({ application: this });
    },

    _defineStyleAndThemes : function() {
        let provider = new Gtk.CssProvider();
        provider.load_from_path(Path.STYLE_DIR + 'gtk-style.css');
        Gtk.StyleContext.add_provider_for_screen(Gdk.Screen.get_default(),
                                                 provider,
                                                 600);

        let settings = Gtk.Settings.get_default();
        settings.gtk_application_prefer_dark_theme = true;
    },

    Close: function() {
        this._mainWindow.close();
    },

    ShowFile : function(uri, xid, closeIfAlreadyShown) {
	let file = Gio.file_new_for_uri(uri);
	if (closeIfAlreadyShown &&
	    this._mainWindow.file &&
	    this._mainWindow.file.equal(file)) {
	    this._mainWindow.close();
	    return;
	}
        this._mainWindow.setParent(xid);
        this._mainWindow.setFile(file);
    }
});
