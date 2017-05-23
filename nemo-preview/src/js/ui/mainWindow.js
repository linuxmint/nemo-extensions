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

imports.gi.versions.GdkX11 = '3.0';

const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;
const GObject = imports.gi.GObject;
const Gdk = imports.gi.Gdk;

let screen = Gdk.Screen.get_default()
let mon = screen.get_primary_monitor()
let s = screen.get_monitor_scale_factor(mon)

// TODO: This is the only way to tell clutter what scale to use until
// our next clutter version
GLib.setenv("CLUTTER_SCALE", Math.max(s, 1).toString(), true);

const GdkX11 = imports.gi.GdkX11;
const GdkPixbuf = imports.gi.GdkPixbuf;
const Gtk = imports.gi.Gtk;
const GtkClutter = imports.gi.GtkClutter;
const Clutter = imports.gi.Clutter;
const Pango = imports.gi.Pango;

const Cairo = imports.cairo;
const Tweener = imports.ui.tweener;
const Lang = imports.lang;

const Mainloop = imports.mainloop;

const MimeHandler = imports.ui.mimeHandler;
const Constants = imports.util.constants;
const SpinnerBox = imports.ui.spinnerBox;
const Utils = imports.ui.utils;

const NemoPreview = imports.gi.NemoPreview;

function MainWindow(args) {
    this._init(args);
}

MainWindow.prototype = {
    _init : function(args) {
        args = args || {};

        this._background = null;
        this._isFullScreen = false;
        this._pendingRenderer = null;
        this._renderer = null;
        this._texture = null;
        this._toolbarActor = null;
        this._fullScreenId = 0;
        this._toolbarId = 0;
        this._unFullScreenId = 0;

        this._mimeHandler = new MimeHandler.MimeHandler();

        this._application = args.application;
        this._createGtkWindow();
        this._createClutterEmbed();

	this.file = null;
    },

    _createGtkWindow : function() {
        this._clientDecorated = false;

        this._gtkWindow = new Gtk.Window({ type: Gtk.WindowType.TOPLEVEL,
                                           focusOnMap: true,
                                           decorated: !this._clientDecorated,
                                           hasResizeGrip: false,
                                           skipPagerHint: true,
                                           skipTaskbarHint: true,
                                           windowPosition: Gtk.WindowPosition.CENTER,
                                           gravity: Gdk.Gravity.CENTER,
                                           application: this._application });

        let screen = Gdk.Screen.get_default();
        this._gtkWindow.set_visual(screen.get_rgba_visual());

        this._gtkWindow.connect('delete-event',
                                Lang.bind(this, this._onWindowDeleteEvent));
        this._gtkWindow.connect('realize', Lang.bind(this,
            function() {
                // don't support maximize and minimize
                this._gtkWindow.get_window().set_functions(Gdk.WMFunction.MOVE |
                                                           Gdk.WMFunction.RESIZE |
                                                           Gdk.WMFunction.CLOSE);
            }));
    },

    _createClutterEmbed : function() {
        let scale = this._gtkWindow.get_scale_factor();

        this._clutterEmbed = new GtkClutter.Embed();
        this._gtkWindow.add(this._clutterEmbed);

        this._clutterEmbed.set_receives_default(true);
        this._clutterEmbed.set_can_default(true);

        this._stage = this._clutterEmbed.get_stage();
        this._stage.set_use_alpha(true);
        this._stage.set_opacity(0);
        this._stage.set_color(new Clutter.Color({ red: 0,
                                                  green: 0,
                                                  blue: 0,
                                                  alpha: 255 }));

        this._mainLayout = new Clutter.BinLayout();
        this._mainGroup = new Clutter.Actor({ layout_manager: this._mainLayout });
        this._mainGroup.add_constraint(
            new Clutter.BindConstraint({ coordinate: Clutter.BindCoordinate.SIZE,
                                         source: this._stage }));
        this._stage.add_actor(this._mainGroup);

        this._gtkWindow.connect('key-press-event',
				Lang.bind(this, this._onKeyPressEvent));

        this._stage.connect('button-press-event',
                            Lang.bind(this, this._onButtonPressEvent));
        this._stage.connect('motion-event',
                            Lang.bind(this, this._onMotionEvent));
    },

    _createSolidBackground: function() {
        if (this._background)
            return;

        this._background = new Clutter.Rectangle();
        this._background.set_opacity(255);
        this._background.set_color(new Clutter.Color({ red: 0,
                                                       green: 0,
                                                       blue: 0,
                                                       alpha: 255 }));

        this._mainLayout.add(this._background, Clutter.BinAlignment.FILL, Clutter.BinAlignment.FILL);
        this._background.lower_bottom();
    },

    _createAlphaBackground: function() {
        if (this._background)
            return;

        if (this._clientDecorated) {
            this._background = NemoPreview.create_rounded_background();
        } else {
            this._background = new Clutter.Rectangle();
            this._background.set_color(new Clutter.Color({ red: 0,
                                                           green: 0,
                                                           blue: 0,
                                                           alpha: 255 }));
        }

        this._background.set_opacity(Constants.VIEW_BACKGROUND_OPACITY);
        this._mainLayout.add(this._background, Clutter.BinAlignment.FILL, Clutter.BinAlignment.FILL);

        this._background.lower_bottom();
    },

    /**************************************************************************
     ****************** main object event callbacks ***************************
     **************************************************************************/
    _onWindowDeleteEvent : function() {
        this._clearAndQuit();
    },

    _onKeyPressEvent : function(actor, event) {
        let key = event.get_keyval()[1];

        if (key == Gdk.KEY_Escape ||
            key == Gdk.KEY_space ||
            key == Gdk.KEY_q)
            this._clearAndQuit();

        if (key == Gdk.KEY_f ||
            key == Gdk.KEY_F11)
            this.toggleFullScreen();

        return false;
    },

    _onButtonPressEvent : function(actor, event) {
        let win_coords = event.get_coords();

        if ((event.get_source() == this._toolbarActor) ||
            (event.get_source() == this._quitActor) ||
            (event.get_source() == this._texture &&
             !this._renderer.moveOnClick)) {

            if (event.get_source() == this._toolbarActor)
                this._resetToolbar();

            return false;
        }

        let root_coords =
            this._gtkWindow.get_window().get_root_coords(win_coords[0],
                                                         win_coords[1]);

        this._gtkWindow.begin_move_drag(event.get_button(),
                                        root_coords[0],
                                        root_coords[1],
                                        event.get_time());

        return false;
    },

    _onMotionEvent : function() {
        if (this._toolbarActor)
            this._resetToolbar();

        return false;
    },

    /**************************************************************************
     *********************** texture allocation *******************************
     **************************************************************************/
    _getTextureSize : function() {
        let screenSize = [ this._gtkWindow.get_window().get_width(),
                           this._gtkWindow.get_window().get_height() ];
        let yPadding = this._clientDecorated ? Constants.VIEW_PADDING_Y : 0;

        let availableWidth = this._isFullScreen ? screenSize[0] : Constants.VIEW_MAX_W - 2 * Constants.VIEW_PADDING_X;
        let availableHeight = this._isFullScreen ? screenSize[1] : Constants.VIEW_MAX_H - yPadding;

        let textureSize = this._renderer.getSizeForAllocation([availableWidth, availableHeight], this._isFullScreen);

        return textureSize;
    },

    _getWindowSize : function() {
        let textureSize = this._getTextureSize();
        let windowSize = textureSize;
        let yPadding = this._clientDecorated ? Constants.VIEW_PADDING_Y : 0;

        if (textureSize[0] < (Constants.VIEW_MIN - 2 * Constants.VIEW_PADDING_X) &&
            textureSize[1] < (Constants.VIEW_MIN - yPadding)) {
            windowSize = [ Constants.VIEW_MIN, Constants.VIEW_MIN ];
        } else if (!this._isFullScreen) {
            windowSize = [ windowSize[0] + 2 * Constants.VIEW_PADDING_X,
                           windowSize[1] + yPadding ];
        }

        return windowSize;
    },

    _positionTexture : function() {
        let yFactor = 0;

        let textureSize = this._getTextureSize();
        let windowSize = this._getWindowSize();

        if (textureSize[0] < Constants.VIEW_MIN &&
            textureSize[1] < Constants.VIEW_MIN) {
            yFactor = 0.52;
        }

        if (yFactor == 0) {
            if (this._isFullScreen &&
               (textureSize[0] > textureSize[1]))
                yFactor = 0.52;
            else
                yFactor = 0.92;
        }

        this._texture.set_size(textureSize[0], textureSize[1]);
        this._textureYAlign.factor = yFactor;

        if (this._lastWindowSize &&
            windowSize[0] == this._lastWindowSize[0] &&
            windowSize[1] == this._lastWindowSize[1])
            return;

        this._lastWindowSize = windowSize;

        if (!this._isFullScreen) {
            this._gtkWindow.resize(windowSize[0], windowSize[1]);
        }
    },

    _createRenderer : function(file) {
        if (this._renderer) {
            if (this._renderer.clear)
                this._renderer.clear();

            this._renderer = null;
        }

        /* create a temporary spinner renderer, that will timeout and show itself
         * if the loading takes too long.
         */
        this._renderer = new SpinnerBox.SpinnerBox();
        this._renderer.startTimeout();

        file.query_info_async
        (Gio.FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME + ',' +
         Gio.FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
         Gio.FileQueryInfoFlags.NONE,
         GLib.PRIORITY_DEFAULT, null,
         Lang.bind (this,
                    function(obj, res) {
                        try {
                            this._fileInfo = obj.query_info_finish(res);
                            this.setTitle(this._fileInfo.get_display_name());

                            /* now prepare the real renderer */
                            this._pendingRenderer = this._mimeHandler.getObject(this._fileInfo.get_content_type());
                            this._pendingRenderer.prepare(file, this, Lang.bind(this, this._onRendererPrepared));
                        } catch(e) {
                            /* FIXME: report the error */
                            logError(e, 'Error calling prepare() on viewer');
                        }}));
    },

    _onRendererPrepared : function() {
        /* destroy the spinner renderer */
        this._renderer.destroy();

        this._renderer = this._pendingRenderer;
        this._pendingRenderer = null;

        /* generate the texture and toolbar for the new renderer */
        this._createTexture();
        this._createToolbar();
    },

    _createTexture : function() {
        if (this._texture) {
            this._texture.destroy();
            this._texture = null;
        }

        this._texture = this._renderer.render();
        this._textureYAlign =
            new Clutter.AlignConstraint({ source: this._stage,
                                          factor: 0.5,
                                          align_axis: Clutter.AlignAxis.Y_AXIS });
        this._texture.add_constraint(this._textureYAlign);

        this.refreshSize();
        this._mainGroup.add_child(this._texture);
    },

    /**************************************************************************
     ************************** fullscreen ************************************
     **************************************************************************/
    _onStageUnFullScreen : function() {
        this._stage.disconnect(this._unFullScreenId);
        this._unFullScreenId = 0;

	/* We want the alpha background back now */
        this._background.destroy();
        this._background = null;
        this._createAlphaBackground();

        this._textureYAlign.factor = this._savedYFactor;

        let textureSize = this._getTextureSize();
        this._texture.set_size(textureSize[0],
                               textureSize[1]);

        Tweener.addTween(this._mainGroup,
                         { opacity: 255,
                           time: 0.15,
                           transition: 'easeOutQuad'
                         });
        Tweener.addTween(this._titleGroup,
                         { opacity: 255,
                           time: 0.15,
                           transition: 'easeOutQuad'
                         });
    },

    _exitFullScreen : function() {
        this._isFullScreen = false;

        if (this._toolbarActor) {
            this._toolbarActor.set_opacity(0);
            this._removeToolbarTimeout();
        }

        /* wait for the next stage allocation to fade in the texture
         * and background again.
         */
        this._unFullScreenId =
            this._stage.connect('notify::allocation',
                                Lang.bind(this, this._onStageUnFullScreen));

        /* quickly fade out everything,
         * and then unfullscreen the (empty) window.
         */
        Tweener.addTween(this._mainGroup,
                         { opacity: 0,
                           time: 0.10,
                           transition: 'easeOutQuad',
                           onComplete: function() {
                               this._gtkWindow.unfullscreen();
                           },
                           onCompleteScope: this
                         });
    },

    _onStageFullScreen : function() {
        this._stage.disconnect(this._fullScreenId);
        this._fullScreenId = 0;

        /* We want a solid black background */
        this._background.destroy();
        this._background = null;
	this._createSolidBackground();

	/* Fade in everything but the title */
        Tweener.addTween(this._mainGroup,
                         { opacity: 255,
                           time: 0.15,
                           transition: 'easeOutQuad'
                         });

        /* zoom in the texture now */
        this._savedYFactor = this._textureYAlign.factor;
        let yFactor = this._savedFactor;

        if (this._texture.width > this._texture.height)
            yFactor = 0.52;
        else
            yFactor = 0.92;

        let textureSize = this._getTextureSize();

        Tweener.addTween(this._texture,
                         { width: textureSize[0],
                           height: textureSize[1],
                           time: 0.15,
                           transition: 'easeOutQuad'
                         });

        Tweener.addTween(this._textureYAlign,
                         { factor: yFactor,
                           time: 0.15,
                           transition: 'easeOutQuad'
                         });
    },

    _enterFullScreen : function() {
        this._isFullScreen = true;

        if (this._toolbarActor) {
            /* prepare the toolbar */
            this._toolbarActor.set_opacity(0);
            this._removeToolbarTimeout();
        }

        /* wait for the next stage allocation to fade in the texture
         * and background again.
         */
        this._fullScreenId =
            this._stage.connect('notify::allocation',
                                Lang.bind(this, this._onStageFullScreen));

        /* quickly fade out everything,
         * and then fullscreen the (empty) window.
         */
        Tweener.addTween(this._titleGroup,
                         { opacity: 0,
                           time: 0.10,
                           transition: 'easeOutQuad'
                         });
        Tweener.addTween(this._mainGroup,
                         { opacity: 0,
                           time: 0.10,
                           transition: 'easeOutQuad',
                           onComplete: function () {
                               this._gtkWindow.fullscreen();
                           },
                           onCompleteScope: this
                         });
    },

    /**************************************************************************
     ************************* toolbar helpers ********************************
     **************************************************************************/
    _createToolbar : function() {
        if (this._toolbarActor) {
            this._removeToolbarTimeout();
            this._toolbarActor.destroy();
            this._toolbarActor = null;
        }

        if (this._renderer.createToolbar)
            this._toolbarActor = this._renderer.createToolbar();

        if (!this._toolbarActor)
            return;

        Utils.alphaGtkWidget(this._toolbarActor.get_widget());

        this._toolbarActor.set_reactive(true);
        this._toolbarActor.set_opacity(0);

        this._toolbarActor.margin_bottom = Constants.TOOLBAR_SPACING;
        this._toolbarActor.margin_left = Constants.TOOLBAR_SPACING;
        this._toolbarActor.margin_right = Constants.TOOLBAR_SPACING;

        this._mainLayout.add(this._toolbarActor,
                             Clutter.BinAlignment.CENTER, Clutter.BinAlignment.END);
    },

    _removeToolbarTimeout: function() {
        if (this._toolbarId != 0) {
            Mainloop.source_remove(this._toolbarId);
            this._toolbarId = 0;
        }
    },

    _resetToolbar : function() {
        if (this._toolbarId == 0) {
            Tweener.removeTweens(this._toolbarActor);

            this._toolbarActor.raise_top();
            this._toolbarActor.set_opacity(0);

            Tweener.addTween(this._toolbarActor,
                             { opacity: 200,
                               time: 0.1,
                               transition: 'easeOutQuad'
                             });
        }

        this._removeToolbarTimeout();
        this._toolbarId = Mainloop.timeout_add(1500,
                                               Lang.bind(this,
                                                         this._onToolbarTimeout));
    },

    _onToolbarTimeout : function() {
        this._toolbarId = 0;
        Tweener.addTween(this._toolbarActor,
                         { opacity: 0,
                           time: 0.25,
                           transition: 'easeOutQuad'
                         });
        return false;
    },


    /**************************************************************************
     ************************ titlebar helpers ********************************
     **************************************************************************/
    _createTitle : function() {
        if (this._titleGroup) {
            this._titleGroup.raise_top();
            return;
        }

        this._titleGroupLayout = new Clutter.BoxLayout();
        this._titleGroup =  new Clutter.Box({ layout_manager: this._titleGroupLayout });
        this._stage.add_actor(this._titleGroup);

        this._titleGroup.add_constraint(
            new Clutter.BindConstraint({ source: this._stage,
                                         coordinate: Clutter.BindCoordinate.WIDTH }));

        // if we don't draw client decorations, just create an empty actor
        if (!this._clientDecorated)
            return;

        this._titleLabel = new Gtk.Label({ label: '',
					   ellipsize: Pango.EllipsizeMode.END,
                                           margin: 6 });
        this._titleLabel.get_style_context().add_class('np-decoration');

        this._titleLabel.show();
        this._titleActor = new GtkClutter.Actor({ contents: this._titleLabel });
        Utils.alphaGtkWidget(this._titleActor.get_widget());

        this._quitButton =
            new Gtk.Button({ image: new Gtk.Image ({ icon_size: Gtk.IconSize.MENU,
                                                     icon_name: 'window-close-symbolic' })});
        this._quitButton.get_style_context().add_class('np-decoration');
        this._quitButton.show();

        this._quitButton.connect('clicked',
                                 Lang.bind(this,
                                           this._clearAndQuit));

        this._quitActor = new GtkClutter.Actor({ contents: this._quitButton });
        Utils.alphaGtkWidget(this._quitActor.get_widget());
        this._quitActor.set_reactive(true);

        let hidden = new Clutter.Actor();
        let size = this._quitButton.get_preferred_size()[1];
        hidden.set_size(size.width, size.height);

        this._titleGroupLayout.pack(hidden, false, false, false,
                                    Clutter.BoxAlignment.START, Clutter.BoxAlignment.START);
        this._titleGroupLayout.pack(this._titleActor, true, true, false,
                                    Clutter.BoxAlignment.CENTER, Clutter.BoxAlignment.START);
        this._titleGroupLayout.pack(this._quitActor, false, false, false,
                                    Clutter.BoxAlignment.END, Clutter.BoxAlignment.START);
    },

    /**************************************************************************
     *********************** Window move/fade helpers *************************
     **************************************************************************/

    _clearAndQuit : function() {
        if (this._renderer.clear)
            this._renderer.clear();

        this._gtkWindow.destroy();
    },

    /**************************************************************************
     ************************ public methods **********************************
     **************************************************************************/
    setParent : function(xid) {
        this._parent = NemoPreview.create_foreign_window(xid);
        this._gtkWindow.realize();
        this._gtkWindow.get_window().move_to_current_desktop();

        if (this._parent)
            this._gtkWindow.get_window().set_transient_for(this._parent);

        this._gtkWindow.show_all();
    },

    setFile : function(file) {
        this.file = file;
        this._createAlphaBackground();
        this._createRenderer(file);
        this._createTexture();
        this._createToolbar();
        this._createTitle();

        this._gtkWindow.show_all();
    },

    setTitle : function(label) {
        if (this._clientDecorated)
            this._titleLabel.set_label(label);

        this._gtkWindow.set_title(label);
    },

    refreshSize : function() {
        this._positionTexture();
    },

    toggleFullScreen : function() {
        if (!this._renderer.canFullScreen)
            return;

        if (this._isFullScreen) {
            this._exitFullScreen();
        } else {
            this._enterFullScreen();
        }
    },

    close : function() {
        this._clearAndQuit();
    }
}
