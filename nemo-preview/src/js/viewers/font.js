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

const MimeHandler = imports.ui.mimeHandler;
const Utils = imports.ui.utils;

const Lang = imports.lang;

const GtkClutter = imports.gi.GtkClutter;
const Gtk = imports.gi.Gtk;
const GLib = imports.gi.GLib;
const NemoPreview = imports.gi.NemoPreview;

function FontRenderer(args) {
    this._init(args);
}

FontRenderer.prototype = {
    _init : function(args) {
        this.moveOnClick = true;
        this.canFullScreen = true;
    },

    prepare : function(file, mainWindow, callback) {
        this._mainWindow = mainWindow;
        this._file = file;
        this._callback = callback;

        this._fontWidget = new NemoPreview.FontWidget({ uri: file.get_uri() });
        this._fontWidget.show();
        this._fontWidget.connect('loaded',
                                 Lang.bind(this, this._onFontLoaded));

        this._fontActor = new GtkClutter.Actor({ contents: this._fontWidget });
        Utils.alphaGtkWidget(this._fontActor.get_widget());
    },

    _onFontLoaded : function() {
        this._callback();
    },

    render : function() {
        return this._fontActor;
    },

    getSizeForAllocation : function(allocation) {
        let size = [ this._fontWidget.get_preferred_size()[1].width,
                     this._fontWidget.get_preferred_size()[1].height ];

        if (size[0] > allocation[0])
            size[0] = allocation[0];

        if (size[1] > allocation[1])
            size[1] = allocation[1];

        return size;
    }
}

let handler = new MimeHandler.MimeHandler();
let renderer = new FontRenderer();

let mimeTypes = [
    'application/x-font-ttf',
    'application/x-font-otf',
    'application/x-font-pcf',
    'application/x-font-type1'
];

handler.registerMimeTypes(mimeTypes, renderer);
