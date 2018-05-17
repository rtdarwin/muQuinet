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

#ifndef MUQUINETD_MUX_POLLER_H
#define MUQUINETD_MUX_POLLER_H

#include <memory>
#include <vector>

class SelectableChannel;

class Poller
{
public:
    Poller();
    ~Poller();
    // Non-copyable, Non-moveable
    Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;
    Poller(Poller&&) = delete;
    Poller& operator=(Poller&&) = delete;

    void addChannel(SelectableChannel*);
    void removeChannel(SelectableChannel*);
    void updateChannel(SelectableChannel*);

    void poll(int timeoutMs,
              std::vector<SelectableChannel*>& activeChannelList);

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_MUX_POLLER_H
