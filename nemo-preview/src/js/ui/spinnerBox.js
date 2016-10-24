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

const Gettext = imports.gettext.domain('nemo-preview');
const _ = Gettext.gettext;
const Gtk = imports.gi.Gtk;
const GtkClutter = imports.gi.GtkClutter;

const Tweener = imports.ui.tweener;
const Lang = imports.lang;
const Mainloop = imports.mainloop;

let SPINNER_SIZE = 48;
let TIMEOUT = 500;

function SpinnerBox(args) {
    this._init(args);
}

SpinnerBox.prototype = {
    _init : function(args) {
        this._timeoutId = 0;

        this.canFullScreen = false;
        this.moveOnClick = true;

        this._spinnerBox = Gtk.Box.new(Gtk.Orientation.VERTICAL, 12);
        this._spinnerBox.show();

        this._spinner = Gtk.Spinner.new();
        this._spinner.show();
        this._spinner.set_size_request(SPINNER_SIZE, SPINNER_SIZE);
        this._spinnerBox.pack_start(this._spinner, true, true, 0);

        this._label = new Gtk.Label();
        this._label.set_text(_("Loading..."));
        this._label.show();
        this._spinnerBox.pack_start(this._label, true, true, 0);

        this.actor = new GtkClutter.Actor({ contents: this._spinnerBox });
        this.actor.set_opacity(0);
    },

    render : function() {
        return this.actor;
    },

    getSizeForAllocation : function() {
        let spinnerSize = this._spinnerBox.get_preferred_size();
        return [ spinnerSize[0].width,
                 spinnerSize[0].height ];
    },

    startTimeout : function() {
        if (this._timeoutId)
            return;

        this._spinner.start();
        this._timeoutId = Mainloop.timeout_add(TIMEOUT,
                                               Lang.bind(this,
                                                         this._onTimeoutCompleted));
    },

    destroy : function() {
        if (this._timeoutId) {
            Mainloop.source_remove(this._timeoutId);
            this._timeoutId = 0;
        }

        Tweener.addTween(this.actor,
                         { opacity: 0,
                           time: 0.15,
                           transition: 'easeOutQuad',
                           onComplete: function() {
                               this.actor.destroy();
                           },
                           onCompleteScope: this
                         });
    },

    _onTimeoutCompleted : function() {
        this._timeoutId = 0;

        Tweener.addTween(this.actor,
                         { opacity: 255,
                           time: 0.3,
                           transition: 'easeOutQuad'
                         });
    },
}
