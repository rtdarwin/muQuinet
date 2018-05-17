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

#ifndef MUQUINETD_SOCKETBUFFER_H
#define MUQUINETD_SOCKETBUFFER_H

#include <memory>

class SocketBuffer
{
public:
    std::shared_ptr<SocketBuffer> next;

    char* transport_hdr = nullptr; // length in [8, 60]
    char* network_hdr = nullptr;   // length in [20, 60]
    char* hdrs_begin = nullptr;
    char* hdrs_end = nullptr;
    char* user_payload_begin = nullptr; // user payload in [begin, end)
    char* user_payload_end = nullptr;

    // 1522 = 18 (ethernet frame headoff 6+6+2+4) + 1500 (ethernet mtu)
    // 18 bytes is reserved for TAP device, TUN device don't need these.
    static const int maxRawBytesSize = 1518;
    char rawBytes[maxRawBytesSize];
};

#endif // MUQUINETD_SOCKETBUFFER_H
