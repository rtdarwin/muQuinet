/*
 * muQuinet, an userspace TCP/IP network stack.
 * Copyright (C) 2018 rtdarwin
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef INTERCEPTOR_FD2CHANNEL_H
#define INTERCEPTOR_FD2CHANNEL_H

#include <stdbool.h>

#include "rpc/c_out/request.pb-c.h"
#include "rpc/c_out/response.pb-c.h"

typedef int channel;

void fd2channel_module_init();

// for interceptor use
bool is_assigned_by_muquinet(int fd);
channel fd2channel(int fd);
void set_fd2channel(int fd, channel ch);
void unset_fd2channel(int fd, channel ch);
channel get_proc_channel();

#endif // INTERCEPTOR_FD2CHANNEL_H
