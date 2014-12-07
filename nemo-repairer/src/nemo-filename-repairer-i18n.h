/*
 * Nemo Filename Repairer Extension
 *
 * Copyright (C) 2008 Choe Hwanjin
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

#ifndef nemo_filename_repairer_i18n_h
#define nemo_filename_repairer_i18n_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* remove warning */
#undef _
#undef N_

#ifdef ENABLE_NLS

#include <libintl.h>
#define _(String) dgettext(GETTEXT_PACKAGE, String)
#define N_(String) gettext_noop (String)

#else /* ENABLE_NLS */

#define _(String) (String)
#define N_(String) String
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain,String) (String)
#define dcgettext(Domain,String,Type) (String)
#define bindtextdomain(Domain,Directory) (Domain)
#define bind_textdomain_codeset(Domain,Encoding) (Domain)

#endif /* ENABLE_NLS */

#define gettext_noop(String) String

#endif /* nemo_filename_repairer_i18n_h */
