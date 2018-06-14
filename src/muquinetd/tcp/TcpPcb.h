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

#ifndef MUQUINETD_TCP_TCPPCB_H
#define MUQUINETD_TCP_TCPPCB_H

#include "muquinetd/Pcb.h"
#include "muquinetd/mux/SelectableChannel.h"
#include "muquinetd/tcp/TcpHeader.h"

typedef uint32_t tcp_seq;

class TcpPcb : public Pcb
{
public:
    // let compiler generate ctor/dctor
    TcpPcb();

    // override functions
    virtual int connect(const struct in_addr& faddr, __be16 fport) override;
    virtual void setOnConnEstabCB(const std::function<void()>&) override;
    virtual SelectableChannel* getConnEstabNotifyChannel() override;
    virtual void disconnect() override;
    virtual int send(const std::string& buf) override;
    virtual void recv(const std::shared_ptr<SocketBuffer>&) override;

    virtual __be16 nextAvailLocalPort() override;

private:
    std::shared_ptr<SocketBuffer> socketBufferOfTcpTempl();
    void prepareBeforeIpTx(const std::shared_ptr<SocketBuffer>&,
                           const std::string& buf = std::string());
    void ack2peer(tcp_seq);

private:
    constexpr static int msl = 30; // 30s

    short conn_state = TcpState::TCP_STATE__CLOSED;

    /* connect */
    int estab_notify_pipe[2];
    std::function<void()> onConnEstabCB;
    std::unique_ptr<SelectableChannel> sChannel;

    short send_flags = 0;

    /* send sequence */
    const static tcp_seq iss = 1;
    tcp_seq send_unack;
    tcp_seq send_next;
    tcp_seq peer_recv_adv;

    /* receive sequence */
    // tcp_seq rcv_wnd;
    tcp_seq irs;
    tcp_seq recv_next;

    /* congestion control */
    // TODO
};

#endif
