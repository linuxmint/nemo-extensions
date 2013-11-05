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

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <glib.h>

#include "hash-lib-linux.h"
#include "hash-lib.h"
#include "hash-func.h"

#define LIB_DATA ((struct hash_lib_linux_s *)func->lib_data)

struct hash_lib_linux_s {
	const char *name;
	int sockfd, connfd;
};

G_GNUC_NORETURN static void gtkhash_hash_lib_linux_error(
	struct hash_func_s *func, const char *str)
{
	g_error("%s: %s (%s)", LIB_DATA->name, str, g_strerror(errno));
}

static const char *gtkhash_hash_lib_linux_get_name(const enum hash_func_e id)
{
	switch (id) {
		case HASH_FUNC_MD4:
			return "md4";
		case HASH_FUNC_MD5:
			return "md5";
		case HASH_FUNC_RIPEMD128:
			return "rmd128";
		case HASH_FUNC_RIPEMD160:
			return "rmd160";
		case HASH_FUNC_RIPEMD256:
			return "rmd256";
		case HASH_FUNC_RIPEMD320:
			return "rmd320";
		case HASH_FUNC_SHA1:
			return "sha1";
		case HASH_FUNC_SHA224:
			return "sha224";
		case HASH_FUNC_SHA256:
			return "sha256";
		case HASH_FUNC_SHA384:
			return "sha384";
		case HASH_FUNC_SHA512:
			return "sha512";
		case HASH_FUNC_TIGER128:
			return "tgr128";
		case HASH_FUNC_TIGER160:
			return "tgr160";
		case HASH_FUNC_TIGER192:
			return "tgr192";
		case HASH_FUNC_WHIRLPOOL:
			return "wp512";
		default:
			return NULL;
	}
}

bool gtkhash_hash_lib_linux_is_supported(const enum hash_func_e id)
{
	struct hash_lib_linux_s data;

	if (!(data.name = gtkhash_hash_lib_linux_get_name(id)))
		return false;

	if ((data.sockfd = socket(AF_ALG, SOCK_SEQPACKET, 0)) == -1) {
		g_warning("kernel hash alg '%s' unavailable", data.name);
		return false;
	}

	close(data.sockfd);

	return true;
}

void gtkhash_hash_lib_linux_start(struct hash_func_s *func)
{
	func->lib_data = g_new(struct hash_lib_linux_s, 1);

	struct sockaddr_alg addr = {
		.salg_family = AF_ALG,
		.salg_type = "hash",
	};

	LIB_DATA->name = gtkhash_hash_lib_linux_get_name(func->id);
	g_assert(LIB_DATA->name);
	strcpy((char *)addr.salg_name, LIB_DATA->name);

	LIB_DATA->sockfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
	if (G_UNLIKELY(LIB_DATA->sockfd == -1))
		gtkhash_hash_lib_linux_error(func, "create socket failed");

	if (G_UNLIKELY(bind(LIB_DATA->sockfd, (struct sockaddr *)&addr,
		sizeof(addr)) == -1))
	{
		gtkhash_hash_lib_linux_error(func, "bind failed");
	}

	LIB_DATA->connfd = accept(LIB_DATA->sockfd, NULL, NULL);
	if (G_UNLIKELY(LIB_DATA->connfd == -1))
		gtkhash_hash_lib_linux_error(func, "accept failed");
}

void gtkhash_hash_lib_linux_update(struct hash_func_s *func,
	const uint8_t *buffer, const size_t size)
{
	ssize_t send_size = send(LIB_DATA->connfd, buffer, size, MSG_MORE);

	if (G_UNLIKELY(send_size != (ssize_t)size))
		gtkhash_hash_lib_linux_error(func, "write failed");
}

void gtkhash_hash_lib_linux_stop(struct hash_func_s *func)
{
	close(LIB_DATA->connfd);
	close(LIB_DATA->sockfd);
	g_free(LIB_DATA);
}

uint8_t *gtkhash_hash_lib_linux_finish(struct hash_func_s *func, size_t *size)
{
	uint8_t buf[64 + 1];
	ssize_t read_size = read(LIB_DATA->connfd, buf, sizeof(buf));

	g_assert(read_size < (ssize_t)sizeof(buf));

	if (G_UNLIKELY(read_size == -1))
		gtkhash_hash_lib_linux_error(func, "read failed");

	close(LIB_DATA->connfd);
	close(LIB_DATA->sockfd);
	g_free(LIB_DATA);

	uint8_t *digest = g_memdup(buf, read_size);
	*size = read_size;

	return digest;
}
