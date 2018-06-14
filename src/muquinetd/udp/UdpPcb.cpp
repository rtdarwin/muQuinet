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

#include "muquinetd/udp/UdpPcb.h"

#include <arpa/inet.h>
#include <memory>

#include "muquinetd/Ip.h"
#include "muquinetd/IpHeaderOverlay.h"
#include "muquinetd/Logging.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/Udp.h"
#include "muquinetd/mux/Socket.h"
#include "muquinetd/udp/UdpHeader.h"

using std::shared_ptr;
using std::make_shared;

namespace {

__be16
nextAvailPort()
{
    // FIXME
    static uint16_t port = 1024;
    ++port;
    return __cpu_to_be16(port);
}

} // namespace {

__be16
UdpPcb::nextAvailLocalPort()
{
    return nextAvailPort();
}

int
UdpPcb::send(const std::string& buf)
{
    // Super class first
    Pcb::send(buf);

    /*  SocketBuffer 链中第一个 SocketBuffer
     *   -  SocketBuffer 链中剩余部分由 IP 层分片时组建，此时
     *      只组建第一个 SocketBuffer 即可
     */

    const auto& skbuf_head = make_shared<SocketBuffer>();

    // SocketBuffer 内各 Header 指针
    {
        bzero(skbuf_head->rawBytes, 68);

        skbuf_head->hdrs_begin =
            skbuf_head->rawBytes + 40; // 40 reserved for IP options
        skbuf_head->network_hdr = skbuf_head->hdrs_begin;
        skbuf_head->transport_hdr = skbuf_head->network_hdr + 20;
        skbuf_head->hdrs_end = skbuf_head->hdrs_begin + 28;
    }

    IpHeaderOverlay* ipovly = (IpHeaderOverlay*)skbuf_head->network_hdr;
    UdpHeader* udphdr = (UdpHeader*)skbuf_head->transport_hdr;

    // 传输层设置的 ``与计算 UDP checksum 有关的'' IP Header
    {
        ipovly->protocol = 17; // 17 stands for UDP
        ipovly->protocol_len = htons(buf.length() + 8);
        memcpy(&ipovly->saddr, &this->laddr, sizeof(__be32));
        memcpy(&ipovly->daddr, &this->faddr, sizeof(__be32));
    }

    // UDP Header
    {
        udphdr->source = this->lport;
        udphdr->dest = this->fport;
        udphdr->len = htons(buf.length() + 8);
        bzero(&udphdr->check, sizeof(__be16)); // FIXME: UDP checksum
    }

    // 传输层设置的 ``与计算 UDP checksum 无关的'' IP Header
    {
        ipovly->ttl = 64;
        ipovly->tos = 0;
        ipovly->len = htons(buf.length() + 28);
    }

    // logging
    {
        char src_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        char dest_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        inet_ntop(AF_INET, &ipovly->saddr, src_ipaddr_p, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ipovly->daddr, dest_ipaddr_p, INET_ADDRSTRLEN);

        MUQUINETD_LOG(info)
            << "Packet {source = " << src_ipaddr_p << ":"
            << ntohs(udphdr->source) << ", dest = " << dest_ipaddr_p << ":"
            << ntohs(udphdr->dest) << "}, passing to IP Layer...";
    }

    ipovly->protocol_len = 0; // don't forget this (iphdr->checksum)
    Ip::get()->tx(skbuf_head, buf);

    // 假装没有错误发生
    return buf.length();
}

int
UdpPcb::send(const struct in_addr& faddr, __be16 fport, const std::string& buf)
{
    // Super class first
    Pcb::send(faddr, fport, buf);

    connect(faddr, fport);
    int ret = this->send(buf);
    disconnect();

    return ret;
}

void
UdpPcb::recv(struct sockaddr_in& peeraddr,
             const std::shared_ptr<SocketBuffer>& skbuf)
{
    // Super class first
    Pcb::recv(peeraddr, skbuf);

    skbuf->user_payload_begin = skbuf->transport_hdr + 8;

    // FIXME: race condition with EventLoop thread
    shared_ptr<Socket> socket = this->socket().lock();
    socket->putToRecvQ(peeraddr, skbuf);
}
