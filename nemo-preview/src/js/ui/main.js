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

imports.gi.versions.Gtk = '3.0';

const Format = imports.format;
const Gettext = imports.gettext;

const GLib = imports.gi.GLib;
const Gtk = imports.gi.Gtk;

const Application = imports.ui.application;
const Path = imports.util.path;
const Utils = imports.ui.utils;
const Tweener = imports.ui.tweener;

function run(argv) {
    Gettext.bindtextdomain('nemo-extensions', Path.LOCALE_DIR);
    String.prototype.format = Format.format;

    GLib.set_application_name('NemoPreview');

    let slowdownEnv = GLib.getenv('NEMO_PREVIEW_SLOWDOWN_FACTOR');
    if (slowdownEnv) {
        let factor = parseFloat(slowdownEnv);
        if (!isNaN(factor) && factor > 0.0)
            Utils.setSlowDownFactor(factor);
    }

    Tweener.init();

    let application = new Application.Application();
    return application.run(null);
}
