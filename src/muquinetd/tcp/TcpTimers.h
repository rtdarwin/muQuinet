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

#ifndef MUQUINETD_TCP_TCPTIMERS_H
#define MUQUINETD_TCP_TCPTIMERS_H

#include <ev.h>

class TcpPcb;

// standard-layout
struct TcpTimers
{
    ev_timer estab;
    ev_timer retransmission;
    ev_timer delay_ack;
    ev_timer persist;  // not implemented
    ev_timer keeplive; // not implemented
    ev_timer fin_wait_2_state;
    ev_timer time_wait_state;

    TcpPcb* pcb;
};

#endif
