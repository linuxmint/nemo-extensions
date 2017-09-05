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

const GdkPixbuf = imports.gi.GdkPixbuf;
const GtkClutter = imports.gi.GtkClutter;
const Gtk = imports.gi.Gtk;
const GLib = imports.gi.GLib;

const Gettext = imports.gettext.domain('nemo-preview');
const _ = Gettext.gettext;
const Lang = imports.lang;
const Mainloop = imports.mainloop;

const MimeHandler = imports.ui.mimeHandler;
const Utils = imports.ui.utils;

function ImageRenderer(args) {
    this._init(args);
}

ImageRenderer.prototype = {
    _init : function(args) {
        this._timeoutId = 0;
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
        GdkPixbuf.PixbufAnimation.new_from_stream_async
        (stream, null,
         Lang.bind(this, function(obj, res) {
             let anim = GdkPixbuf.PixbufAnimation.new_from_stream_finish(res);

             this._iter = anim.get_iter(null);
             let pix = this._iter.get_pixbuf().apply_embedded_orientation();

             this._texture = new GtkClutter.Texture({ keep_aspect_ratio: true });
             this._texture.set_from_pixbuf(pix);

             if (!anim.is_static_image())
                 this._startTimeout();

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

    _startTimeout : function() {
        this._timeoutId = Mainloop.timeout_add(this._iter.get_delay_time(),
                                               Lang.bind(this,
                                                         this._advanceImage));
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

    destroy : function () {
        /* We should do the check here because it is possible
         * that we never created a source if our image is
         * not animated. */
        if (this._timeoutId) {
            Mainloop.source_remove(this._timeoutId);
            this._timeoutId = 0;
        }
    },

    _advanceImage : function () {
        this._iter.advance(null);
        let pix = this._iter.get_pixbuf().apply_embedded_orientation();
        this._texture.set_from_pixbuf(pix);
        return true;
    },
}

let handler = new MimeHandler.MimeHandler();
let renderer = new ImageRenderer();

let formats = GdkPixbuf.Pixbuf.get_formats();
for (let idx in formats) {
    let mimeTypes = formats[idx].get_mime_types();
    handler.registerMimeTypes(mimeTypes, renderer);
}
