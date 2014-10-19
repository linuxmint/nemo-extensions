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

const Gdk = imports.gi.Gdk;
const Gtk = imports.gi.Gtk;

const Lang = imports.lang;

const Constants = imports.util.constants;

let slowDownFactor = 0;

function setSlowDownFactor(factor) {
    slowDownFactor = factor;
}

function getScaledSize(baseSize, allocSize, upscale) {
    let allocW = allocSize[0];
    let allocH = allocSize[1];
    let width = baseSize[0];
    let height = baseSize[1];
    let scale = 1.0;

    if (((width <= allocW && height <= allocH) && upscale) ||
        (width > allocW && height > allocH)) {
        /* up/downscale both directions */
        let allocRatio = allocW / allocH;
        let baseRatio = width / height;

        if (baseRatio > allocRatio)
            scale = allocW / width;
        else
            scale = allocH / height;
    } else if (width > allocW &&
               height <= allocH) {
        /* downscale x */
        scale = allocW / width;
    } else if (width <= allocW &&
               height > allocH) {
        /* downscale y */
        scale = allocH / height;
    }

    width *= scale;
    height *= scale;

    return [ Math.floor(width), Math.floor(height) ];
}

function getStaticSize(renderer, widget) {
    let width = widget.get_preferred_width()[1];
    let height = widget.get_preferred_height()[1];

    if (width < Constants.VIEW_MIN &&
        height < Constants.VIEW_MIN) {
        width = Constants.VIEW_MIN;
    }

    /* never make it shrink; this could happen when the
     * spinner hides.
     */
    if (width < renderer.lastWidth)
        width = renderer.lastWidth;
    else
        renderer.lastWidth = width;

    if (height < renderer.lastHeight)
        height = renderer.lastHeight;
    else
        renderer.lastHeight = height;

    /* return the natural */
    return [ width, height ];
}

function createToolButton(iconName, callback) {
    let button = new Gtk.ToolButton({ expand: false,
                                      icon_name: iconName });
    button.show();
    button.connect('clicked', callback);

    return button;
}

function createFullScreenButton(mainWindow) {
    return createToolButton('view-fullscreen-symbolic', Lang.bind(this, function() {
        mainWindow.toggleFullScreen();
    }));
}

function createOpenButton(file, mainWindow) {
    return createToolButton('document-open-symbolic', Lang.bind(this, function(widget) {
        let timestamp = Gtk.get_current_event_time();
        try {
            Gtk.show_uri(widget.get_screen(),
                         file.get_uri(),
                         timestamp);

            mainWindow.close();
        } catch (e) {
            log('Unable to execute the default application for ' +
                file.get_uri() + ' : ' + e.toString());
        }
    }));
}

function formatTimeString(timeVal) {
    let hours = Math.floor(timeVal / 3600);
    timeVal -= hours * 3600;

    let minutes = Math.floor(timeVal / 60);
    timeVal -= minutes * 60;

    let seconds = Math.floor(timeVal);

    let str = ('%02d:%02d').format(minutes, seconds);
    if (hours > 0) {
        str = ('%d').format(hours) + ':' + str;
    }

    return str;
}

function alphaGtkWidget(widget) {
    widget.override_background_color(0, new Gdk.RGBA({ red: 0,
                                                       green: 0,
                                                       blue: 0,
                                                       alpha: 0 }));
}
