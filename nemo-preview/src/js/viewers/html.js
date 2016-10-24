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

const GtkClutter = imports.gi.GtkClutter;
const Gtk = imports.gi.Gtk;
const GLib = imports.gi.GLib;
const WebKit = imports.gi.WebKit2;
const NemoPreview = imports.gi.NemoPreview;

const MimeHandler = imports.ui.mimeHandler;
const Utils = imports.ui.utils;

function HTMLRenderer(args) {
    this._init(args);
}

HTMLRenderer.prototype = {
    _init : function(args) {
        this.moveOnClick = false;
        this.canFullScreen = true;
    },

    prepare : function(file, mainWindow, callback) {
        this._mainWindow = mainWindow;
        this._file = file;
        this._callback = callback;

        this._webView = new WebKit.WebView();
        this._webView.show_all();

        /* disable the default context menu of the web view */
        this._webView.connect ("context-menu",
                               function() {return true;});

        this._webView.load_uri(file.get_uri());

        this._actor = new GtkClutter.Actor({ contents: this._webView });
        this._actor.set_reactive(true);

        this._callback();
    },

    render : function() {
        return this._actor;
    },

    getSizeForAllocation : function(allocation) {
        return allocation;
    },

    createToolbar : function() {
        this._mainToolbar = new Gtk.Toolbar({ icon_size: Gtk.IconSize.MENU });
        this._mainToolbar.get_style_context().add_class('osd');
        this._mainToolbar.set_show_arrow(false);
        this._mainToolbar.show();

        this._toolbarActor = new GtkClutter.Actor({ contents: this._mainToolbar });

        this._toolbarZoom = Utils.createFullScreenButton(this._mainWindow);
        this._mainToolbar.insert(this._toolbarZoom, 0);

        let separator = new Gtk.SeparatorToolItem();
        separator.show();
        this._mainToolbar.insert(separator, 1);

        this._toolbarRun = Utils.createOpenButton(this._file, this._mainWindow);
        this._mainToolbar.insert(this._toolbarRun, 2);

        return this._toolbarActor;
    }
};

let handler = new MimeHandler.MimeHandler();
let renderer = new HTMLRenderer();

let mimeTypes = [
    'text/html'
];

handler.registerMimeTypes(mimeTypes, renderer);
