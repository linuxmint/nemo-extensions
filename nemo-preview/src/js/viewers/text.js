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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 *
 * The NemoPreview project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and NemoPreview. This
 * permission is above and beyond the permissions granted by the GPL license
 * NemoPreview is covered by.
 *
 * Authors: Cosimo Cecchi <cosimoc@redhat.com>
 *
 */

imports.gi.versions.GtkSource = '3.0';

const GtkClutter = imports.gi.GtkClutter;
const Gtk = imports.gi.Gtk;
const GLib = imports.gi.GLib;
const GtkSource = imports.gi.GtkSource;
const Gio = imports.gi.Gio;
const NemoPreview = imports.gi.NemoPreview;

const MimeHandler = imports.ui.mimeHandler;
const Utils = imports.ui.utils;

const Lang = imports.lang;

function TextRenderer(args) {
    this._init(args);
}

TextRenderer.prototype = {
    _init : function(args) {
        this.moveOnClick = false;
        this.canFullScreen = true;
    },

    prepare : function(file, mainWindow, callback) {
        this._mainWindow = mainWindow;
        this._file = file;
        this._callback = callback;

        this._textLoader = new NemoPreview.TextLoader();
        this._textLoader.connect('loaded',
                                 Lang.bind(this, this._onBufferLoaded));
        this._textLoader.uri = file.get_uri();

        this._geditScheme = 'tango';
        let schemaName = 'org.x.editor.preferences.editor';
        let installedSchemas = Gio.Settings.list_schemas();
        if (installedSchemas.indexOf(schemaName) > -1) {
            let geditSettings = new Gio.Settings({ schema: schemaName });
            let geditSchemeName = geditSettings.get_string('scheme');
            if (geditSchemeName != '')
                this._geditScheme = geditSchemeName;
        }

    },

    render : function() {
        return this._actor;
    },

    _onBufferLoaded : function(loader, buffer) {
        this._buffer = buffer;
        this._buffer.highlight_syntax = true;

        let styleManager = GtkSource.StyleSchemeManager.get_default();
        let scheme = styleManager.get_scheme(this._geditScheme);
        this._buffer.set_style_scheme(scheme);

        this._view = new GtkSource.View({ buffer: this._buffer,
                                          editable: false,
                                          cursor_visible: false });
        this._view.set_can_focus(false);

        if (this._buffer.get_language())
            this._view.set_show_line_numbers(true);

        // FIXME: *very* ugly wokaround to the fact that we can't
        // access event.button from a button-press callback to block
        // right click
        this._view.connect('populate-popup',
                           Lang.bind(this, function(widget, menu) {
                               menu.destroy();
                           }));

        this._scrolledWin = Gtk.ScrolledWindow.new(null, null);
        this._scrolledWin.add(this._view);
        this._scrolledWin.show_all();

        this._actor = new GtkClutter.Actor({ contents: this._scrolledWin });
        this._actor.set_reactive(true);
        this._callback();
    },

    getSizeForAllocation : function(allocation) {
        return allocation;
    },

    createToolbar : function() {
        this._mainToolbar = new Gtk.Toolbar({ icon_size: Gtk.IconSize.MENU });
        this._mainToolbar.get_style_context().add_class('osd');
        this._mainToolbar.set_show_arrow(false);

        this._toolbarRun = Utils.createOpenButton(this._file, this._mainWindow);
        this._mainToolbar.insert(this._toolbarRun, 0);

        this._mainToolbar.show();

        this._toolbarActor = new GtkClutter.Actor({ contents: this._mainToolbar });

        return this._toolbarActor;
    }
}

let handler = new MimeHandler.MimeHandler();
let renderer = new TextRenderer();

/* register for text/plain and let the mime handler call us
 * for child types.
 */
let mimeTypes = [
    'text/plain'
];

handler.registerMimeTypes(mimeTypes, renderer);
