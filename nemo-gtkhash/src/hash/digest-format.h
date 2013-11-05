/*
 *   Copyright (C) 2007-2013 Tristan Heaven <tristanheaven@gmail.com>
 *
 *   This file is part of GtkHash.
 *
 *   GtkHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   GtkHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GtkHash. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GTKHASH_HASH_DIGEST_FORMAT_H
#define GTKHASH_HASH_DIGEST_FORMAT_H

#define DIGEST_FORMATS_N 3
#define DIGEST_FORMAT_IS_VALID(X) (((X) >= 0) && ((X) < DIGEST_FORMATS_N))

enum digest_format_e {
	DIGEST_FORMAT_INVALID = -1,
	DIGEST_FORMAT_HEX_LOWER,
	DIGEST_FORMAT_HEX_UPPER,
	DIGEST_FORMAT_BASE64,
};

#endif
