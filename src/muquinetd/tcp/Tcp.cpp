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

#include "muquinetd/Tcp.h"

#include <netinet/in.h>
#include <string.h>

#include <algorithm>
#include <list>

#include "muquinetd/IpHeaderOverlay.h"
#include "muquinetd/Logging.h"
#include "muquinetd/Pcb.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/mux/Socket.h"
#include "muquinetd/tcp/TcpPcb.h"

using std::unique_ptr;
using std::make_shared;
using std::shared_ptr;
using std::weak_ptr;

struct Tcp::Impl
{
    std::list<weak_ptr<Pcb>> pcbs;
};

Tcp::Tcp()
{
    _pImpl.reset(new Tcp::Impl);
}

Tcp::~Tcp() = default;

void
Tcp::init()
{
    // Nothing need to do
}

void
Tcp::start()
{
    // Nothing need to do
}

void
Tcp::stop()
{
}

shared_ptr<Pcb>
Tcp::newPcb()
{
    const auto& pcb = make_shared<TcpPcb>();
    _pImpl->pcbs.push_back(weak_ptr<Pcb>(pcb));

    return pcb;
}

void
Tcp::removePcb(const std::shared_ptr<const Pcb>& pcb)
{
    // TOOD
}

void
Tcp::rx(const shared_ptr<SocketBuffer>& skbuf_head)
{
    /*  1. len & checksum */

    // TODO

    /*  2. demultiplex */

    IpHeaderOverlay* iphdr_ovly = nullptr;
    TcpHeader* tcphdr = nullptr;

    struct in_addr faddr, laddr;
    __be16 fport, lport;

    iphdr_ovly = (IpHeaderOverlay*)skbuf_head->network_hdr;
    tcphdr = (TcpHeader*)skbuf_head->transport_hdr;
    memcpy(&faddr, &iphdr_ovly->saddr, sizeof(struct in_addr));
    memcpy(&laddr, &iphdr_ovly->daddr, sizeof(struct in_addr));
    fport = tcphdr->source;
    lport = tcphdr->dest;

    shared_ptr<Pcb> pcb;
    pcb = Pcbs::find(_pImpl->pcbs, faddr, fport, laddr, lport);
    if (!pcb) {
        // TODO
        // ICMP Type3 Destination Unreachable
        {
            MUQUINETD_LOG(info) << "No receiver, this TCP packet"
                                   " {source port = "
                                << __be16_to_cpu(fport)
                                << ", dest port = " << __be16_to_cpu(lport)
                                << "} will be droped ";
        }
        return;
    }

    pcb->recv(skbuf_head);
    {
        MUQUINETD_LOG(info) << "TCP Layer Delivered a TCP packet {sport = "
                            << ntohs(tcphdr->source)
                            << ", dport = " << ntohs(tcphdr->dest) << "}";
    }
}
