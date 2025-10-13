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
imports.gi.versions.ClutterGst = '3.0';

const Gtk = imports.gi.Gtk;
const GdkPixbuf = imports.gi.GdkPixbuf;
const Gio = imports.gi.Gio;
const Gst = imports.gi.Gst;
const GtkClutter = imports.gi.GtkClutter;
const NemoPreview = imports.gi.NemoPreview;

const Gettext = imports.gettext.domain('nemo-extensions');
const _ = Gettext.gettext;
const Lang = imports.lang;

const Constants = imports.util.constants;
const MimeHandler = imports.ui.mimeHandler;
const TotemMimeTypes = imports.util.totemMimeTypes;
const Utils = imports.ui.utils;

function AudioRenderer(args) {
    this._init(args);
}

AudioRenderer.prototype = {
    _init : function() {
        this.moveOnClick = true;
        this.canFullScreen = false;
    },

    prepare : function(file, mainWindow, callback) {
        this._mainWindow = mainWindow;
        this._file = file;
        this._callback = callback;

        this._createPlayer(file);

        this._box = new Gtk.Box({ orientation: Gtk.Orientation.HORIZONTAL,
                                  spacing: 6 });
        this._image = new Gtk.Image({ icon_name: 'xapp-media-optical-symbolic',
                                      pixel_size: 256 });
        this._box.pack_start(this._image, false, false, 0);

        let vbox = new Gtk.Box({ orientation: Gtk.Orientation.VERTICAL,
                                 spacing: 1,
                                 margin_top: 48,
                                 margin_left: 12,
                                 margin_right: 12 });
        this._box.pack_start(vbox, false, false, 0);

        this._titleLabel = new Gtk.Label();
        this._titleLabel.set_halign(Gtk.Align.START);
        vbox.pack_start(this._titleLabel, false, false, 0);

        this._authorLabel = new Gtk.Label();
        this._authorLabel.set_halign(Gtk.Align.START);
        vbox.pack_start(this._authorLabel, false, false, 0);

        this._albumLabel = new Gtk.Label();
        this._albumLabel.set_halign(Gtk.Align.START);
        vbox.pack_start(this._albumLabel, false, false, 0);

        this._box.show_all();
        this._actor = new GtkClutter.Actor({ contents: this._box });

        this._callback();
    },

    render : function() {
        return this._actor;
    },

    _createPlayer : function(file) {
        this._playerNotifies = [];

        this._player = new NemoPreview.SoundPlayer({ uri: file.get_uri() });
        this._player.playing = true;

        this._playerNotifies.push(
            this._player.connect('notify::progress',
                                 Lang.bind(this, this._onPlayerProgressChanged)));
        this._playerNotifies.push(
            this._player.connect('notify::duration',
                                 Lang.bind(this, this._onPlayerDurationChanged)));
        this._playerNotifies.push(
            this._player.connect('notify::state',
                                 Lang.bind(this, this._onPlayerStateChanged)));
        this._playerNotifies.push(
            this._player.connect('notify::taglist',
                                 Lang.bind(this, this._onTagListChanged)));
        this._playerNotifies.push(
            this._player.connect('notify::cover',
                                 Lang.bind(this, this._onCoverArtChanged)));
    },

    clear : function(file) {
        this._playerNotifies.forEach(Lang.bind(this,
            function(id) {
                this._player.disconnect(id);
            }));

        this._player.playing = false;
        this._playerNotifies = [];
        this._player = null;
    },

    _ensurePixbufSize : function(cover) {
        let width, height;

        width = cover.get_width();
        height = cover.get_height();

        if (width > 256 ||
            height > 256) {
            if (width > height) {
                this._coverArt = cover.scale_simple(256,
                                                    height * 256 / width,
                                                    GdkPixbuf.InterpType.BILINEAR);
            } else {
                this._coverArt = cover.scale_simple(width * 256 / height,
                                                    256,
                                                    GdkPixbuf.InterpType.BILINEAR);
            }
        } else {
            this._coverArt = cover;
        }
    },

    _onCoverArtChanged : function() {
        if (!this._artFetcher.cover) {
            this._image.set_from_icon_name('xapp-media-optical-symbolic');
            return;
        }

        this._ensurePixbufSize(this._artFetcher.cover);
        this._image.set_from_pixbuf(this._coverArt);
    },

    _onTagListChanged : function() {
        let tags = this._player.taglist;
        let albumName = tags.get_string('album')[1];
        let artistName = tags.get_string('artist')[1];
        let titleName = tags.get_string('title')[1];
        let haveTitleTag = true;

        if (!titleName) {
            let file = Gio.file_new_for_uri(this._player.uri);
            titleName = file.get_basename();
            haveTitleTag = false;
        }

        if (albumName)
            this._albumLabel.set_markup('<small><i>' + _("from") + '  </i>' + albumName + '</small>');
        if (artistName)
            this._authorLabel.set_markup('<small><i>' + _("by") + '  </i><b>' + artistName + '</b></small>');

        this._titleLabel.set_markup('<b>' + titleName + '</b>');

        let windowTitle = '';

        if (artistName && haveTitleTag)
            windowTitle = artistName + ' - ' + titleName;
        else
            windowTitle = titleName;

        this._mainWindow.setTitle(windowTitle);

        this._artFetcher = new NemoPreview.CoverArtFetcher();
        this._artFetcher.connect('notify::cover',
                                 Lang.bind(this, this._onCoverArtChanged));

        this._artFetcher.taglist = tags;

        this._mainWindow.refreshSize();
    },

    _updateProgressBar : function() {
        if (!this._mainToolbar)
            return;

        this._isSettingValue = true;
        this._progressBar.set_value(this._player.progress * 1000);
        this._isSettingValue = false;
    },

    _updateCurrentLabel : function() {
        if (!this._mainToolbar)
            return;

        let currentTime =
            Math.floor(this._player.duration * this._player.progress);

        this._currentLabel.set_text(Utils.formatTimeString(currentTime));
    },

    _updateDurationLabel : function() {
        if (!this._mainToolbar)
            return;

        let totalTime = this._player.duration;

        this._durationLabel.set_text(Utils.formatTimeString(totalTime));
    },

    _onPlayerProgressChanged : function() {
        this._updateProgressBar();
        this._updateCurrentLabel();
    },

    _onPlayerDurationChanged : function() {
        this._updateDurationLabel();
    },

    _onPlayerStateChanged : function() {
        switch(this._player.state) {
        case NemoPreview.SoundPlayerState.PLAYING:
            this._toolbarPlay.set_icon_name('xapp-media-playback-pause-symbolic');
            break;
        default:
            this._toolbarPlay.set_icon_name('xapp-media-playback-start-symbolic');
        }
    },

    getSizeForAllocation : function(allocation) {
        let width = this._box.get_preferred_width();
        let height = this._box.get_preferred_height();

        if (width[1] < Constants.VIEW_MIN &&
            height[1] < Constants.VIEW_MIN) {
            width[1] = Constants.VIEW_MIN;
        }

        /* return the natural */
        return [ width[1], height[1] ];
    },

    createToolbar : function () {
        this._mainToolbar = new Gtk.Toolbar();
        this._mainToolbar.get_style_context().add_class('osd');
        this._mainToolbar.set_icon_size(Gtk.IconSize.MENU);
        this._mainToolbar.show();

        this._toolbarActor = new GtkClutter.Actor({ contents: this._mainToolbar,
                                                    opacity: 0,
                                                    x_expand: true });

        this._toolbarPlay = new Gtk.ToolButton({ icon_name: 'xapp-media-playback-pause-symbolic' });
        this._toolbarPlay.show();
        this._mainToolbar.insert(this._toolbarPlay, 0);

        this._currentLabel = new Gtk.Label({ margin_left: 6,
                                             margin_right: 3 });
        let item = new Gtk.ToolItem();
        item.add(this._currentLabel);
        item.show_all();
        this._mainToolbar.insert(item, 1);

        this._toolbarPlay.connect('clicked',
                                  Lang.bind(this, function () {
                                      let playing = !this._player.playing;
                                      this._player.playing = playing;
                                  }));

        this._progressBar =
            Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL,
                                     0, 1000, 10);
        this._progressBar.set_value(0);
        this._progressBar.set_draw_value(false);
        this._progressBar.connect('value-changed',
                                  Lang.bind(this, function() {
                                      if(!this._isSettingValue)
                                          this._player.progress = this._progressBar.get_value() / 1000;
                                  }));

        item = new Gtk.ToolItem();
        item.set_expand(true);
        item.add(this._progressBar);
        item.show_all();
        this._mainToolbar.insert(item, 2);

        this._durationLabel = new Gtk.Label({ margin_left: 3 });
        item = new Gtk.ToolItem();
        item.add(this._durationLabel);
        item.show_all();
        this._mainToolbar.insert(item, 3);

        return this._toolbarActor;
    },
}

let handler = new MimeHandler.MimeHandler();
let renderer = new AudioRenderer();

handler.registerMimeTypes(TotemMimeTypes.audioTypes, renderer);
