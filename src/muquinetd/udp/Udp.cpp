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

#include "muquinetd/Udp.h"

#include <netinet/in.h>
#include <string.h>

#include <algorithm>
#include <list>

// #include "muquinetd/base/ConcurrentDeque.h"
#include "muquinetd/IpHeaderOverlay.h"
#include "muquinetd/Logging.h"
#include "muquinetd/Pcb.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/udp/UdpHeader.h"
#include "muquinetd/udp/UdpPcb.h"

using std::unique_ptr;
using std::make_shared;
using std::weak_ptr;
using std::shared_ptr;

struct Udp::Impl
{
    // FIXME: weak_ptr
    // Udp::rx
    std::list<weak_ptr<Pcb>> pcbs;
};

Udp::Udp()
{
    _pImpl.reset(new Udp::Impl);
}

Udp::~Udp() = default;

void
Udp::init()
{
    // Nothing need to do
}

void
Udp::start()
{
    // Nothing need to do
}

void
Udp::stop()
{
}

shared_ptr<Pcb>
Udp::newPcb()
{
    const auto& pcb = make_shared<UdpPcb>();
    const auto& weak_pcb = pcb;
    _pImpl->pcbs.push_back(weak_pcb);

    return pcb;
}

void
Udp::removePcb(const std::shared_ptr<const Pcb>& pcb)
{
    // TODO
}

void
Udp::rx(const shared_ptr<SocketBuffer>& skbuf_head)
{
    /*  1. len & checksum */

    // TODO

    /*  2. demultiplex */

    IpHeaderOverlay* iphdr_ovly = nullptr;
    UdpHeader* udphdr = nullptr;

    struct in_addr faddr, laddr;
    __be16 fport, lport;

    iphdr_ovly = (IpHeaderOverlay*)skbuf_head->network_hdr;
    udphdr = (UdpHeader*)skbuf_head->transport_hdr;
    memcpy(&faddr, &iphdr_ovly->saddr, sizeof(struct in_addr));
    memcpy(&laddr, &iphdr_ovly->daddr, sizeof(struct in_addr));
    fport = udphdr->source;
    lport = udphdr->dest;

    shared_ptr<Pcb> pcb;
    pcb = Pcbs::find(_pImpl->pcbs, faddr, fport, laddr, lport);
    if (!pcb) {
        // TODO
        // ICMP Type3 Destination Unreachable
        {
            MUQUINETD_LOG(info) << "No receiver, this UDP packet"
                                   " {source port = "
                                << __be16_to_cpu(fport)
                                << ", dest port = " << __be16_to_cpu(lport)
                                << "} will be droped ";
        }
        return;
    }

    sockaddr_in peeraddr;
    bzero(&peeraddr, 0);
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_addr = *(struct in_addr*)&iphdr_ovly->saddr;
    peeraddr.sin_port = udphdr->source;

    pcb->recv(peeraddr, skbuf_head);

    {
        MUQUINETD_LOG(info) << "UDP Layer Delivered an UDP packet {sport = "
                            << ntohs(udphdr->source)
                            << ", dport = " << ntohs(udphdr->dest) << "}";
    }
}
