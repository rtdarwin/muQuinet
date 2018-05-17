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

#include "muquinetd/Ip.h"

#include <arpa/inet.h>
#include <boost/thread/thread.hpp>
#include <memory>

#include "muquinetd/Interface.h"
#include "muquinetd/Logging.h"
// #include "muquinetd/base/ConcurrentDeque.h"
#include "muquinetd/Tcp.h"
#include "muquinetd/Udp.h"
#include "muquinetd/base/InternetChecksum.h"
#include "muquinetd/ip/Defrager.h"
#include "muquinetd/ip/IpHeader.h"
#include "third-party/concurrentqueue/blockingconcurrentqueue.h"

using std::unique_ptr;
using std::make_shared;
using std::shared_ptr;
using boost::thread;
using moodycamel::BlockingConcurrentQueue;

struct Ip::Impl
{
    BlockingConcurrentQueue<shared_ptr<SocketBuffer>> rxQ;
    static const int qSizeLimit = 2048;
    bool stopflag = false;

    unique_ptr<Defrager> defrager;
    unique_ptr<IpIdGenerator> idgenerator;
};

// Reason that defaulted ctor/dtor definitions here:
//  https://stackoverflow.com/questions/9954518/stdunique-ptr-with-an-incomplete-type-wont-compile/32269374
Ip::Ip()
{
    _pImpl.reset(new Ip::Impl);
}

Ip::~Ip() = default;

void
Ip::init()
{
    _pImpl->defrager.reset(new Defrager);
    _pImpl->idgenerator.reset(new IpIdGenerator);
}

void
Ip::start()
{
    MUQUINETD_LOG(debug) << "Starting IP Layer rx_loop_thread";
    thread rxLoop([this]() {
        LoggingThreadInitializer i;
        i.run();

        char saddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        char daddr_p[INET_ADDRSTRLEN + 1] = { 0 };

        MUQUINETD_LOG(info) << "IP Layer rx_loop_thread begins to work";
        shared_ptr<SocketBuffer> skbuf;
        IpHeader* iphdr = nullptr;
        while (!this->_pImpl->stopflag) {
            _pImpl->rxQ.wait_dequeue(skbuf);

            if (!skbuf)
                continue;

            {
                MUQUINETD_LOG(info) << "IP Layer take a packet from IP rxQ";
            }

            assert(skbuf->hdrs_begin && skbuf->user_payload_end &&
                   skbuf->network_hdr);

            /*  1. checksum */

            if (skbuf->user_payload_end - skbuf->hdrs_begin < 20 ||
                !IpHeaders::checksum(iphdr)) {
                // FIXME: drop this packet
                continue;
            }

            /*  2. defrag */

            iphdr = (IpHeader*)skbuf->network_hdr;

            {
                inet_ntop(AF_INET, (void*)&iphdr->saddr, saddr_p,
                          INET_ADDRSTRLEN);
                inet_ntop(AF_INET, (void*)&iphdr->daddr, daddr_p,
                          INET_ADDRSTRLEN);
                MUQUINETD_LOG(info)
                    << "New IP packet = {saddr = " << std::string(saddr_p)
                    << ", daddr = " << std::string(daddr_p) << "}";
            }

            if (IpHeaders::isFrag(iphdr)) {
                {
                    int off = __be16_to_cpu(iphdr->frag_off) & IP_OFF_MASK;
                    MUQUINETD_LOG(info)
                        << "New IP packet is a fragment, we will defrag it. "
                        << "{frag offset(8n) = 8*" << off << "}";
                }
                skbuf = _pImpl->defrager->defrag(skbuf);
            }

            /*  3. demultiplex */

            if (!skbuf)
                continue;

            int trans_hdr_offset = iphdr->ihl * 4;
            skbuf->transport_hdr = skbuf->network_hdr + trans_hdr_offset;

            using ProtoX = IpHeaders::ProtocolX;
            switch (IpHeaders::protocol(iphdr)) {
                case ProtoX::IP:
                    break;
                case ProtoX::ICMP: // TODO
                    MUQUINETD_LOG(info) << "New IP packet is a ICMP packet, "
                                           "passing it to ICMP Layer...";
                    break;
                case ProtoX::IGMP: // TODO
                    MUQUINETD_LOG(info)
                        << "New IP packet is a IGMP packet, we will drop it";
                    break;
                case ProtoX::UDP:
                    MUQUINETD_LOG(info) << "New IP packet is a UDP packet, "
                                           "passing it to UDP Layer...";
                    Udp::get()->rx(skbuf);
                    break;
                case ProtoX::TCP:
                    MUQUINETD_LOG(info) << "New IP packet is a TCP packet, "
                                           "passing it to TCP Layer...";
                    Tcp::get()->rx(skbuf);
                    break;
            }

            // Don't forget this
            skbuf.reset();
        }
    });
    MUQUINETD_LOG(debug) << "Started IP Layer rx_loop_thread";
}

void
Ip::stop()
{
    MUQUINETD_LOG(info) << "Stopping IP Layer";
    _pImpl->stopflag = true;
}

void
Ip::enRxQue(const std::shared_ptr<SocketBuffer>& skbuf)
{
    IpHeader* iphdr = (IpHeader*)skbuf->network_hdr;
    if (iphdr->version != 4) {
        MUQUINETD_LOG(warning) << "IPv6 not implemented!";
        return;
    }

    _pImpl->rxQ.enqueue(skbuf);
    MUQUINETD_LOG(info) << "Ip Layer received a packet, put it into rxQ";
    MUQUINETD_LOG(debug) << "Ip rxQue size = " << _pImpl->rxQ.size_approx();
}

void
Ip::tx(const std::shared_ptr<SocketBuffer>& skbuf_head,
       const std::string& user_payload)
{
    assert(skbuf_head->hdrs_begin && skbuf_head->hdrs_end);

    /*  options */
    // TODO

    /*  第一个 SocketBuffer */

    IpHeader* iphdr = (IpHeader*)skbuf_head->network_hdr;
    iphdr->version = 4;
    iphdr->ihl = 20 / 4;
    iphdr->id = _pImpl->idgenerator->next();
    iphdr->check = InternetChecksum::checksum(iphdr, iphdr->ihl * 4, 0);

    skbuf_head->user_payload_begin = (char*)user_payload.c_str();
    skbuf_head->user_payload_end =
        (char*)user_payload.c_str() + user_payload.length();

    /*  后续 SocketBuffer（分片） */
    // TODO

    MUQUINETD_LOG(info) << "Fraging (if needed) finished, passing packet(s) to "
                           "Interface Layer...";
    Interface::get()->tx(skbuf_head);
}
