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

const GdkPixbuf = imports.gi.GdkPixbuf;
const GtkClutter = imports.gi.GtkClutter;
const Gtk = imports.gi.Gtk;
const GLib = imports.gi.GLib;

const Gettext = imports.gettext.domain('nemo-preview');
const _ = Gettext.gettext;
const Lang = imports.lang;

const MimeHandler = imports.ui.mimeHandler;
const Utils = imports.ui.utils;

function ImageRenderer(args) {
    this._init(args);
}

ImageRenderer.prototype = {
    _init : function(args) {
        this.moveOnClick = true;
        this.canFullScreen = true;
    },

    prepare : function(file, mainWindow, callback) {
        this._mainWindow = mainWindow;
        this._file = file;
        this._callback = callback;

        this._createImageTexture(file);
    },

    render : function() {
        return this._texture;
    },

    _createImageTexture : function(file) {
        file.read_async
        (GLib.PRIORITY_DEFAULT, null,
         Lang.bind(this,
                   function(obj, res) {
                       try {
                           let stream = obj.read_finish(res);
                           this._textureFromStream(stream);
                       } catch (e) {
                       }
                   }));
    },

    _textureFromStream : function(stream) {
        GdkPixbuf.Pixbuf.new_from_stream_async
        (stream, null,
         Lang.bind(this, function(obj, res) {
             let pix = GdkPixbuf.Pixbuf.new_from_stream_finish(res);
             pix.apply_embedded_orientation();

             this._texture = new GtkClutter.Texture({ keep_aspect_ratio: true });
             this._texture.set_from_pixbuf(pix);

             /* we're ready now */
             this._callback();

             stream.close_async(GLib.PRIORITY_DEFAULT,
                                null, function(object, res) {
                                    try {
                                        object.close_finish(res);
                                    } catch (e) {
                                        log('Unable to close the stream ' + e.toString());
                                    }
                                });
         }));
    },

    getSizeForAllocation : function(allocation, fullScreen) {
        let baseSize = this._texture.get_base_size();
        return Utils.getScaledSize(baseSize, allocation, fullScreen);
    },

    createToolbar : function() {
        this._mainToolbar = new Gtk.Toolbar({ icon_size: Gtk.IconSize.MENU });
        this._mainToolbar.get_style_context().add_class('osd');
        this._mainToolbar.set_show_arrow(false);
        this._mainToolbar.show();

        this._toolbarActor = new GtkClutter.Actor({ contents: this._mainToolbar });

        this._toolbarZoom = Utils.createFullScreenButton(this._mainWindow);
        this._mainToolbar.insert(this._toolbarZoom, 0);

        return this._toolbarActor;
    },
}

let handler = new MimeHandler.MimeHandler();
let renderer = new ImageRenderer();

let formats = GdkPixbuf.Pixbuf.get_formats();
for (idx in formats) {
    let mimeTypes = formats[idx].get_mime_types();
    handler.registerMimeTypes(mimeTypes, renderer);
}
