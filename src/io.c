/*
 * io.c - non-blocking I/O operations
 * Copyright (C) 2015  Vivien Didelot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "io.h"
#include "log.h"
#include "sys.h"

/* Read a single character and return a negative error code if none was read */
static int io_getchar(int fd, char *c)
{
	return sys_read(fd, c, 1, NULL);
}

/* Read a line including the newline character and return its positive length */
static ssize_t io_getline(int fd, char *buf, size_t size)
{
	size_t len = 0;
	int err;

	for (;;) {
		if (len == size)
			return -ENOSPC;

		err = io_getchar(fd, buf + len);
		if (err)
			return err;

		if (buf[len++] == '\n')
			break;
	}

	/* at least 1 */
	return len;
}

/* Read a line excluding the newline character */
static int io_readline(int fd, io_line_cb *cb, size_t num, void *data)
{
	char buf[BUFSIZ];
	ssize_t len;
	int err;

	len = io_getline(fd, buf, sizeof(buf));
	if (len < 0)
		return len;

	/* replace newline with terminating null byte */
	buf[len - 1] = '\0';

	if (cb) {
		err = cb(buf, num, data);
		if (err)
			return err;
	}

	return 0;
}

/* Read up to count lines excluding their newline character */
int io_readlines(int fd, size_t count, io_line_cb *cb, void *data)
{
	size_t lines = 0;
	int err;

	while (count--) {
		err = io_readline(fd, cb, lines++, data);
		if (err) {
			if (err == -EAGAIN)
				break;

			/* support end-of-file as well */
			if (err == -EPIPE)
				break;

			return err;
		}
	}

	return 0;
}
