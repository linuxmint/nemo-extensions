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

const EvDoc = imports.gi.XreaderDocument;
const EvView = imports.gi.XreaderView;
const Gtk = imports.gi.Gtk;
const GtkClutter = imports.gi.GtkClutter;
const NemoPreview = imports.gi.NemoPreview;

const Gettext = imports.gettext.domain('nemo-extensions');
const _ = Gettext.gettext;
const Lang = imports.lang;

const Constants = imports.util.constants;
const MimeHandler = imports.ui.mimeHandler;
const Utils = imports.ui.utils;

function XreaderRenderer(args) {
    this._init(args);
}

XreaderRenderer.prototype = {
    _init : function(args) {
        EvDoc.init();

        this._pdfLoader = null;
        this._document = null;

        this.moveOnClick = false;
        this.canFullScreen = true;
    },

    prepare : function(file, mainWindow, callback) {
        this._mainWindow = mainWindow;
        this._file = file;
        this._callback = callback;

        this._pdfLoader = new NemoPreview.PdfLoader();
        this._pdfLoader.connect('notify::document',
                                Lang.bind(this, this._onDocumentLoaded));
        this._pdfLoader.uri = file.get_uri();
    },

    render : function() {
        return this._actor;
    },

    _updatePageLabel : function() {
        let curPage, totPages;

        curPage = this._model.get_page();
        totPages = this._document.get_n_pages();

        this._toolbarBack.set_sensitive(curPage > 0);
        this._toolbarForward.set_sensitive(curPage < totPages - 1);

        this._pageLabel.set_text(_("%d of %d").format(curPage + 1, totPages));
    },

    _onDocumentLoaded : function() {
        this._document = this._pdfLoader.document;
        this._model = EvView.DocumentModel.new_with_document(this._document);

        this._model.set_sizing_mode(EvView.SizingMode.FIT_WIDTH);
	this._model.set_continuous(true);

        this._model.connect('page-changed',
                            Lang.bind(this, function() {
                                this._updatePageLabel();
                            }));

        this._view = EvView.View.new();
        this._view.show();

        this._scrolledWin = Gtk.ScrolledWindow.new(null, null);
        this._scrolledWin.set_min_content_width(Constants.VIEW_MIN);
        this._scrolledWin.set_min_content_height(Constants.VIEW_MIN);
        this._scrolledWin.show();

        this._view.set_model(this._model);
        this._scrolledWin.add(this._view);

        this._actor = new GtkClutter.Actor({ contents: this._scrolledWin });
        this._actor.set_reactive(true);

        this._callback();
    },

    getSizeForAllocation : function(allocation) {
        /* always give the view the maximum possible allocation */
        return allocation;
    },

    _createLabelItem : function() {
        this._pageLabel = new Gtk.Label({ margin_left: 10,
                                          margin_right: 10 });

        let item = new Gtk.ToolItem();
        item.set_expand(true);
        item.add(this._pageLabel);
        item.show_all();

        return item;
    },

    createToolbar : function() {
        this._mainToolbar = new Gtk.Toolbar({ icon_size: Gtk.IconSize.MENU });
        this._mainToolbar.get_style_context().add_class('osd');
        this._mainToolbar.set_show_arrow(false);
        this._mainToolbar.show();

        this._toolbarActor = new GtkClutter.Actor({ contents: this._mainToolbar });

        this._toolbarBack = new Gtk.ToolButton({ expand: false,
                                                 icon_name: 'go-previous-symbolic' });
        this._toolbarBack.show();
        this._mainToolbar.insert(this._toolbarBack, -1);

        this._toolbarBack.connect('clicked',
                                  Lang.bind(this, function () {
                                      this._view.previous_page();
                                  }));

        let labelItem = this._createLabelItem();
        this._mainToolbar.insert(labelItem, -1);

        this._toolbarForward = new Gtk.ToolButton({ expand: false,
                                                    icon_name: 'go-next-symbolic' });
        this._toolbarForward.show();
        this._mainToolbar.insert(this._toolbarForward, -1);

        this._toolbarForward.connect('clicked',
                                     Lang.bind(this, function () {
                                         this._view.next_page();
                                     }));

        let separator = new Gtk.SeparatorToolItem();
        separator.show();
        this._mainToolbar.insert(separator, -1);

        this._toolbarZoom = Utils.createFullScreenButton(this._mainWindow);
        this._mainToolbar.insert(this._toolbarZoom, -1);

        this._updatePageLabel();

        return this._toolbarActor;
    },

    clear : function() {
        this._pdfLoader.cleanup_document();
        this._document = null;
        this._pdfLoader = null;
    }
}

let handler = new MimeHandler.MimeHandler();
let renderer = new XreaderRenderer();

let mimeTypes = NemoPreview.query_supported_document_types();
handler.registerMimeTypes(mimeTypes, renderer);

let officeTypes = [
    'application/vnd.oasis.opendocument.text',
    'application/vnd.oasis.opendocument.presentation',
    'application/vnd.oasis.opendocument.spreadsheet',
    'application/msword',
    'application/vnd.ms-excel',
    'application/vnd.ms-powerpoint',
    'application/rtf'
];

handler.registerMimeTypes(officeTypes, renderer);
