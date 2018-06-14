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

#include "muquinetd/tcp/TcpPcb.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <memory>
#include <unistd.h>

#include "muquinetd/Ip.h"
#include "muquinetd/IpHeaderOverlay.h"
#include "muquinetd/Logging.h"
#include "muquinetd/Mux.h"
#include "muquinetd/SocketBuffer.h"
#include "muquinetd/base/InternetChecksum.h"
#include "muquinetd/mux/EventLoop.h"
#include "muquinetd/mux/Socket.h"
#include "muquinetd/tcp/TcpHeader.h"
#include "muquinetd/tcp/TcpTimers.h"

using std::shared_ptr;
using std::make_shared;
using std::string;

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

TcpPcb::TcpPcb()
    : Pcb()
{
    pipe2(estab_notify_pipe, O_NONBLOCK);
    sChannel.reset(new SelectableChannel(estab_notify_pipe[0]));
    sChannel->enableReading();
    sChannel->setOnReadCB([this]() {
        char readbuf[8];
        while (::read(this->estab_notify_pipe[0], readbuf, 8) != -1)
            ;

        if (onConnEstabCB)
            onConnEstabCB();
    });
}

__be16
TcpPcb::nextAvailLocalPort()
{
    return nextAvailPort();
}

int
TcpPcb::connect(const struct in_addr& faddr, __be16 fport)
{
    // Super class first
    Pcb::connect(faddr, fport);

    // 这里只发送第一个 SYN,
    std::string empty;
    this->send(empty);

    return 0;
}

void
TcpPcb::setOnConnEstabCB(const std::function<void()>& f)
{
    this->onConnEstabCB = f;
}

SelectableChannel*
TcpPcb::getConnEstabNotifyChannel()
{
    return this->sChannel.get();
}

void
TcpPcb::disconnect()
{
    // TODO
    // 四次挥手
}

int
TcpPcb::send(const std::string& buf)
{
    // Super class first
    Pcb::send(buf);

    shared_ptr<SocketBuffer> skbuf_head = this->socketBufferOfTcpTempl();
    TcpHeader* tcphdr = (TcpHeader*)skbuf_head->transport_hdr;

    /*  1. 不同 TCP 状态发不同类型的 TCP 报文 */

    switch (this->conn_state) {
        case TcpState::TCP_STATE__CLOSED:
            this->send_unack = this->send_next = this->iss;
            tcphdr->seq = htonl(send_next), ++send_next;
            tcphdr->syn = 1;

            this->conn_state = TCP_STATE__SYN_SENT; //
            break;
        case TcpState::TCP_STATE__ESTABLISHED:
            tcphdr->seq = htonl(send_next);
            send_next += buf.length();
            tcphdr->ack = 1;
            tcphdr->ack_seq = htonl(recv_next);
            break;
        default:
            // - 客户端的 SYN_SEND, FIN_WAIT_1, FIN_WAIT2
            //   状态在处理异步到达的包时才需要判断
            // - 客户端的 TIME_WAIT 状态由定时器处理
            // - 其他的状态用于服务器，我们不考虑（因为我们不实现服务器）
            return 0;
    }

    this->prepareBeforeIpTx(skbuf_head, buf);
    Ip::get()->tx(skbuf_head, buf);

    // 假装没有错误发生
    return buf.length();
}

void
TcpPcb::recv(const std::shared_ptr<SocketBuffer>& skbuf)
{
    TcpHeader* tcphdr = (TcpHeader*)skbuf->transport_hdr;

    switch (this->conn_state) {
        case TcpState::TCP_STATE__SYN_SENT:
            // FIXME: logging rather than crash
            assert(tcphdr->syn && tcphdr->ack);

            /*  1. recv SYN & ACK */
            this->irs = ntohl(tcphdr->seq);
            this->recv_next = irs + 1;
            ++this->send_unack;
            this->conn_state = TCP_STATE__ESTABLISHED;

            /*  2. sent ACK */

            this->ack2peer(recv_next);

            /*  3. callback */

            if (onConnEstabCB) {
                onConnEstabCB();
            }

            break;
        case TcpState::TCP_STATE__ESTABLISHED: // TODO
            if (tcphdr->ack) {
                uint32_t k = ntohl(tcphdr->ack_seq);
                if (k > this->send_unack) {
                    this->send_unack = ntohl(tcphdr->ack_seq);
                }
            }
            skbuf->user_payload_begin = skbuf->transport_hdr + tcphdr->doff * 4;
            if (skbuf->user_payload_end - skbuf->user_payload_begin) {
                // Save to Socket buffer
                struct sockaddr_in peer_addr;
                shared_ptr<Socket> so = this->socket().lock();
                so->putToRecvQ(peer_addr, skbuf);

                // Send ACK
                if (tcphdr->seq != this->recv_next) {
                    MUQUINETD_LOG(warning) << "Out-of-order TCP segment...";
                }
                this->recv_next = ntohl(tcphdr->seq) + skbuf->user_payload_end -
                                  skbuf->user_payload_begin;
                this->ack2peer(this->recv_next);
            } else {
                // do nothing
            }
            break;
        default:
            // - 客户端的 SYN_SEND, FIN_WAIT_1, FIN_WAIT2
            //   状态在处理异步到达的包时才需要判断
            // - 客户端的 TIME_WAIT 状态由定时器处理
            // - 其他的状态用于服务器，我们不考虑（因为我们不实现服务器）
            return;
    }
}

shared_ptr<SocketBuffer>
TcpPcb::socketBufferOfTcpTempl()
{
    /*  1. SocketBuffer */

    /*  SocketBuffer 链中第一个 SocketBuffer
     *   -  SocketBuffer 链中剩余部分由 IP 层分片时组建，此时
     *      只组建第一个 SocketBuffer 即可
     */

    const auto& skbuf_head = make_shared<SocketBuffer>();
    // SocketBuffer 内各 Header 指针
    {
        bzero(skbuf_head->rawBytes, 120); // IP 60 + TCP 60

        skbuf_head->hdrs_begin =
            skbuf_head->rawBytes + 40; // 40 reserved for IP options
        skbuf_head->network_hdr = skbuf_head->hdrs_begin;
        skbuf_head->transport_hdr = skbuf_head->network_hdr + 20;
        skbuf_head->hdrs_end = skbuf_head->transport_hdr + 20;
    }

    /*  2. 通用的 TCP 和 IP 首部 */

    TcpHeader* tcphdr = (TcpHeader*)skbuf_head->transport_hdr;
    {
        tcphdr->source = this->lport;
        tcphdr->dest = this->fport;
        tcphdr->doff = 20 / 4;
        tcphdr->window = htons(4096);
    }

    return skbuf_head;
}

void
TcpPcb::prepareBeforeIpTx(const std::shared_ptr<SocketBuffer>& skbuf_head,
                          const string& buf)
{
    IpHeaderOverlay* ipovly = (IpHeaderOverlay*)skbuf_head->network_hdr;
    TcpHeader* tcphdr = (TcpHeader*)skbuf_head->transport_hdr;

    // 传输层设置的 ``与计算 TCP checksum 有关的'' IP Header
    {
        ipovly->protocol = 6; // 6 stands for TCP
        ipovly->protocol_len = htons(buf.length() + 20);
        memcpy(&ipovly->saddr, &this->laddr, sizeof(__be32));
        memcpy(&ipovly->daddr, &this->faddr, sizeof(__be32));
    }

    // TCP Header
    {
        // uint32_t hdr_checksum = InternetChecksum::checksum(
        //     skbuf_head->network_hdr,
        //     tcphdr->doff * 4 + 20 /* IP Header length*/, 0);
        // if (buf.length()) {
        //     tcphdr->check = InternetChecksum::checksum(
        //         buf.c_str(), buf.length(), hdr_checksum);
        // } else {
        //     tcphdr->check = hdr_checksum;
        // }

        uint32_t sum = 0;
        if (buf.length()) {
            int count = buf.length();

            uint16_t* ptr = (uint16_t*)buf.c_str();
            while (count > 1) {
                /*  This is the inner loop */
                sum += *ptr++;
                count -= 2;
            }

            /*  Add left-over byte, if any */
            if (count > 0)
                sum += *(uint8_t*)ptr;
        }
        tcphdr->check = InternetChecksum::checksum(
            skbuf_head->network_hdr,
            tcphdr->doff * 4 + 20 /* IP Header length*/, sum);
    }

    // 传输层设置的 ``与计算 TCP checksum 无关的'' IP Header
    {
        ipovly->ttl = 64;
        ipovly->tos = 0;
        ipovly->len = htons(buf.length() + 40);
        ipovly->protocol_len = 0; // don't forget this (iphdr->checksum)
    }

    // logging
    {
        char src_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        char dest_ipaddr_p[INET_ADDRSTRLEN + 1] = { 0 };
        inet_ntop(AF_INET, &ipovly->saddr, src_ipaddr_p, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ipovly->daddr, dest_ipaddr_p, INET_ADDRSTRLEN);

        MUQUINETD_LOG(info)
            << "Packet {source = " << src_ipaddr_p << ":"
            << ntohs(tcphdr->source) << ", dest = " << dest_ipaddr_p << ":"
            << ntohs(tcphdr->dest) << "}, passing to IP Layer...";
    }
}

void
TcpPcb::ack2peer(tcp_seq value)
{
    shared_ptr<SocketBuffer> skbuf_head = this->socketBufferOfTcpTempl();
    TcpHeader* tcphdr = (TcpHeader*)skbuf_head->transport_hdr;

    tcphdr->ack = 1;
    tcphdr->ack_seq = htonl(value);
    tcphdr->seq = htonl(this->send_next);

    prepareBeforeIpTx(skbuf_head);
    string empty;
    Ip::get()->tx(skbuf_head, empty);
}
