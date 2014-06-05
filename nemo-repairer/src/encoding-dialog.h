/*
 * Nemo Filename Repairer Extension
 *
 * Copyright (C) 2011 Choe Hwanjin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Author: Choe Hwajin <choe.hwanjin@gmail.com>
 */

#ifndef nemo_filename_repairer_encoding_dialog_h
#define nemo_filename_repairer_encoding_dialog_h

#include <gtk/gtk.h>

GtkDialog* encoding_dialog_new(GtkWindow* parent);
char* encoding_dialog_run(GtkDialog* dialog);

#endif /* nemo_filename_repairer_encoding_dialog_h */
