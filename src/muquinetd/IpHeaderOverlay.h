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

#ifndef MUQUINETD_UDP_IPHEADEROVERLAY_H
#define MUQUINETD_UDP_IPHEADEROVERLAY_H

#include <asm/byteorder.h>

struct IpHeaderOverlay
{
    /* 1 */
    __u8 zeros10;
    __u8 tos;
    __be16 len;
    /* 2 */
    __be32 zeros2;
    /* 3 */
    __u8 ttl;
    __u8 protocol;
    __be16 protocol_len;
    /* 4 */
    __be32 saddr;
    /* 5 */
    __be32 daddr;
};

#endif // MUQUINETD_UDP_IPHEADEROVERLAY_H
