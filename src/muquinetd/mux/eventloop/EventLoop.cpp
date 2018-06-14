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

#include "muquinetd/mux/EventLoop.h"

#include <algorithm>
#include <vector>

#include "muquinetd/Logging.h"
#include "muquinetd/mux/SelectableChannel.h"
#include "muquinetd/mux/eventloop/Poller.h"

using std::shared_ptr;
using std::vector;

struct EventLoop::Impl
{
    bool stopflag = false;

    std::unique_ptr<Poller> poller;
    static const int pollTimeoutMs = -1; // 1ms
    std::vector<SelectableChannel*> channels;
    std::vector<SelectableChannel*> activeChannels;

    bool handingEvents = false;
    // TODO: consult cocos2dx.EventDispatcher
    //       for defering add/remove/update operations to channels
};

EventLoop::EventLoop()
{
    _pImpl.reset(new EventLoop::Impl);
    _pImpl->poller.reset(new Poller);
}

EventLoop::~EventLoop() = default;

void
EventLoop::addChannel(SelectableChannel* c)
{
    MUQUINETD_LOG(info) << "Adding a SelectableChannel";

    if (!this->hasChannel(c)) {
        _pImpl->poller->addChannel(c);
        _pImpl->channels.push_back(c);
    } else {
        MUQUINETD_LOG(error)
            << "Trying to add a Channel already in this EventLoop";
    }
}

void
EventLoop::removeChannel(SelectableChannel* c)
{
    MUQUINETD_LOG(debug) << "Removing a SelectableChannel";

    if (this->hasChannel(c)) {
        _pImpl->poller->removeChannel(c);

        auto& channels = _pImpl->channels;
        auto it = std::find(channels.begin(), channels.end(), c);
        _pImpl->channels.erase(it);

    } else {
        MUQUINETD_LOG(error)
            << "Trying to remove a Channel not in this EventLoop";
    }
}

void
EventLoop::updateChannel(SelectableChannel* c)
{
    MUQUINETD_LOG(debug) << "Modifying a SelectableChannel";

    if (this->hasChannel(c)) {
        _pImpl->poller->updateChannel(c);
    } else {
        MUQUINETD_LOG(error)
            << "Trying to update a Channel not in this EventLoop";
    }
}

bool
EventLoop::hasChannel(SelectableChannel* c)
{
    auto it = std::find(_pImpl->channels.cbegin(), _pImpl->channels.cend(), c);
    if (it != _pImpl->channels.cend()) {
        return true;
    } else {
        return false;
    }
}

void
EventLoop::loop()
{
    _pImpl->stopflag = false;
    while (!_pImpl->stopflag) {
        auto actives = _pImpl->activeChannels;
        actives.clear();
        _pImpl->poller->poll(_pImpl->pollTimeoutMs, actives);

        _pImpl->handingEvents = true;
        for (const auto& c : actives) {
            c->handleEventsReceived();
        }
        _pImpl->handingEvents = false;
    }
}

void
EventLoop::stop()
{
    _pImpl->stopflag = true;
}
