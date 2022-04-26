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
const Gtk = imports.gi.Gtk;

const Gio = imports.gi.Gio;
const GtkClutter = imports.gi.GtkClutter;
const NemoPreview = imports.gi.NemoPreview;

const Gettext = imports.gettext.domain('nemo-extensions');
const _ = Gettext.gettext;
const Lang = imports.lang;

const Constants = imports.util.constants;
const Utils = imports.ui.utils;

function FallbackRenderer(args) {
    this._init(args);
}

FallbackRenderer.prototype = {
    _init : function() {
        this._fileLoader = null;
        this._fileLoaderId = 0;

        this.moveOnClick = true;
        this.canFullScreen = false;
    },

    prepare : function(file, mainWindow, callback) {
        this._mainWindow = mainWindow;
        this.lastWidth = 0;
        this.lastHeight = 0;

        this._fileLoader = new NemoPreview.FileLoader();
        this._fileLoader.file = file;
        this._fileLoaderId =
            this._fileLoader.connect('notify',
                                     Lang.bind(this, this._onFileInfoChanged));

        this._box = new Gtk.Box({ orientation: Gtk.Orientation.HORIZONTAL,
                                  spacing: 6 });
        this._image = new Gtk.Image({ icon_name: 'document',
                                      pixel_size: 256 });
        this._box.pack_start(this._image, false, false, 0);

        let vbox = new Gtk.Box({ orientation: Gtk.Orientation.VERTICAL,
                                 spacing: 1,
                                 margin_top: 48,
                                 margin_left: 12,
                                 margin_right: 12 });
        this._box.pack_start(vbox, false, false, 0);

        let hbox = new Gtk.Box({ orientation: Gtk.Orientation.HORIZONTAL,
                                 spacing: 6 });
        vbox.pack_start(hbox, false, false, 0);

        this._titleLabel = new Gtk.Label();
        this._titleLabel.set_halign(Gtk.Align.START);
        hbox.pack_start(this._titleLabel, false, false, 0);

        this._spinner = new Gtk.Spinner();
        hbox.pack_start(this._spinner, false, false, 0);
        this._spinner.start();

        this._typeLabel = new Gtk.Label();
        this._typeLabel.set_halign(Gtk.Align.START);
        vbox.pack_start(this._typeLabel, false, false, 0);

        this._sizeLabel = new Gtk.Label();
        this._sizeLabel.set_halign(Gtk.Align.START);
        vbox.pack_start(this._sizeLabel, false, false, 0);

        this._dateLabel = new Gtk.Label();
        this._dateLabel.set_halign(Gtk.Align.START);
        vbox.pack_start(this._dateLabel, false, false, 0);

        this._applyLabels();

        this._box.show_all();
        this._actor = new GtkClutter.Actor({ contents: this._box });
        Utils.alphaGtkWidget(this._actor.get_widget());

        callback();
    },

    render : function() {
        return this._actor;
    },

    _applyLabels : function() {
        let titleStr =
            '<b><big>' +
            ((this._fileLoader.name) ? (this._fileLoader.name) : (this._fileLoader.file.get_basename()))
            + '</big></b>';
        this._titleLabel.set_markup(titleStr);

        if (this._fileLoader.get_file_type() != Gio.FileType.DIRECTORY) {
            let typeStr =
                '<small><b>' + _("Type") + '  </b>' +
                ((this._fileLoader.contentType) ? (this._fileLoader.contentType) : (_("Loading...")))
                + '</small>';
            this._typeLabel.set_markup(typeStr);
        } else {
            this._typeLabel.hide();
        }

        let sizeStr =
            '<small><b>' + _("Size") + '  </b>' +
            ((this._fileLoader.size) ? (this._fileLoader.size) : (_("Loading...")))
             + '</small>';
        this._sizeLabel.set_markup(sizeStr);

        let dateStr =
            '<small><b>' + _("Modified") + '  </b>' +
             ((this._fileLoader.time) ? (this._fileLoader.time) : (_("Loading...")))
             + '</small>';
        this._dateLabel.set_markup(dateStr);
    },

    _onFileInfoChanged : function() {
        if (!this._fileLoader.get_loading()) {
            this._spinner.stop();
            this._spinner.hide();
        }

        if (this._fileLoader.icon)
            this._image.set_from_pixbuf(this._fileLoader.icon);
        else
            this._setImageFromType();

        this._applyLabels();
        this._mainWindow.refreshSize();
    },

    clear : function() {
        if (this._fileLoader) {
            this._fileLoader.disconnect(this._fileLoaderId);
            this._fileLoaderId = 0;

            this._fileLoader.stop();
            this._fileLoader = null;
        }
    },

    getSizeForAllocation : function(allocation) {
        return Utils.getStaticSize(this, this._box);
    }
}
