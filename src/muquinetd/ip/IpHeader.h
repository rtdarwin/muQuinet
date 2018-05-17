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

#ifndef MUQUINETD_IP_IPHEADER_H
#define MUQUINETD_IP_IPHEADER_H

#define IP_OFF_MASK 0x1fff
#define IP_DF_MASK 0x4000
#define IP_MF_MASK 0x2000

#include <cstdint>

// Let's just steal from linux
#include <linux/ip.h> // for iphdr
using IpHeader = ::iphdr;

class IpHeaders
{
public:
    enum class ProtocolX
    {
        IP,
        ICMP,
        IGMP,
        UDP,
        TCP,
    };

    /* IP L1 */
    static int hlen(const IpHeader*);

    /* IP L2 */
    static bool isFrag(const IpHeader*);
    static bool moreFrag(const IpHeader*);

    /* IP L3 */
    static ProtocolX protocol(const IpHeader*);
    static bool checksum(const IpHeader*);
};

class IpIdGenerator
{
public:
    // rule of zero

    __be16 next();

private:
    uint16_t last = 0;
};

#endif // MUQUINETD_IP_IPHEADER_H
