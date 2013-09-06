/* -*- mode: js2; js2-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * Copyright (C) 2009-2011 Adel Gadllah <adel.gadllah@gmail.com>
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
 * Authors: Adel Gadllah <adel.gadllah@gmail.com>
 *          Florian Müllner <fmuellner@gnome.org>
 *
 */

/*
 * This function is intended to extend the String object and provide
 * an String.format API for string formatting.
 * It has to be set up using String.prototype.format = Format.format;
 * Usage:
 * "somestring %s %d".format('hello', 5);
 * It supports %s, %d, %x and %f, for %f it also support precisions like
 * "%.2f".format(1.526). All specifiers can be prefixed with a minimum
 * field width, e.g. "%5s".format("foo"). Unless the width is prefixed
 * with '0', the formatted string will be padded with spaces.
 */

function format() {
    let str = this;
    let i = 0;
    let args = arguments;

    return str.replace(/%([0-9]+)?(?:\.([0-9]+))?(.)/g, function (str, widthGroup, precisionGroup, genericGroup) {

                    if (precisionGroup != '' && genericGroup != 'f')
                        throw new Error("Precision can only be specified for 'f'");

                    let fillChar = (widthGroup[0] == '0') ? '0' : ' ';
                    let width = parseInt(widthGroup, 10) || 0;

                    function fillWidth(s, c, w) {
                        let fill = '';
                        for (let i = 0; i < w; i++)
                            fill += c;
                        return fill.substr(s.length) + s;
                    }

                    let s = '';
                    switch (genericGroup) {
                        case '%':
                            return '%';
                            break;
                        case 's':
                            s = args[i++].toString();
                            break;
                        case 'd':
                            s = parseInt(args[i++]).toString();
                            break;
                        case 'x':
                            s = parseInt(args[i++]).toString(16);
                            break;
                        case 'f':
                            if (precisionGroup == '')
                                s = parseFloat(args[i++]).toString();
                            else
                                s = parseFloat(args[i++]).toFixed(parseInt(precisionGroup));
                            break;
                        default:
                            throw new Error('Unsupported conversion character %' + genericGroup);
                    }
                    return fillWidth(s, fillChar, width);
                });
}
