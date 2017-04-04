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

const FallbackRenderer = imports.ui.fallbackRenderer;

const Gio = imports.gi.Gio;

let _mimeHandler = null;

function MimeHandler() {
    if (_mimeHandler == null) {
        this._init();
        _mimeHandler = this;
    }

    return _mimeHandler;
}

function init() {
    let handler = new MimeHandler();
}

MimeHandler.prototype = {
    _init: function() {
        this._mimeTypes = [];

        this._fallbackRenderer = new FallbackRenderer.FallbackRenderer();
    },

    registerMime: function(mime, obj) {
        this._mimeTypes[mime] = obj;

        log ('Register mimetype ' + mime);
    },

    registerMimeTypes: function(mimeTypes, obj) {
        for (let idx in mimeTypes)
            this.registerMime(mimeTypes[idx], obj);
    },

    getObject: function(mime) {
        if (this._mimeTypes[mime]) {
            /* first, try a direct match with the mimetype itself */
            return this._mimeTypes[mime];
        } else {
            /* if this fails, try to see if we have any handlers
             * registered for a parent type.
             */
            for (let key in this._mimeTypes) {
                if (Gio.content_type_is_a (mime, key))
                    return this._mimeTypes[key];
            }

            /* finally, resort to the fallback renderer */
            return this._fallbackRenderer;
        }
    }
}
