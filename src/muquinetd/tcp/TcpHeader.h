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

#ifndef MUQUINETD_TCP_TCPHEADER_H
#define MUQUINETD_TCP_TCPHEADER_H

#include <cstdint>

// Let's just steal from linux
#include <linux/tcp.h>
using TcpHeader = ::tcphdr;

// 不能使用 enum class，因为我们要对这些值进行 > < 运算
enum TcpState
{
    TCP_STATE__CLOSED,
    TCP_STATE__LISTEN,

    // three-way handshake to open connection
    TCP_STATE__SYN_SENT,
    TCP_STATE__SYN_RECEIVED,
    TCP_STATE__ESTABLISHED,

    // Tcp client state in four-way handshake to close connection
    TCP_STATE__FIN_WAIT_1,
    TCP_STATE__FIN_WAIT_2,
    TCP_STATE__CLOSING,
    TCP_STATE__TIME_WAIT,

    // Tcp server state in four-way handshake to close
    TCP_STATE__CLOSE_WAIT,
    TCP_STATE__LAST_ACK,
};

#endif // MUQUINETD_TCP_TCPHEADER_H
