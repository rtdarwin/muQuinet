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

#include "muquinetd/mux/SelectableChannel.h"

#include <fcntl.h>
#include <poll.h>
#include <sys/epoll.h>

#include "muquinetd/Logging.h"
#include "muquinetd/mux/EventLoop.h"

struct EventMasks
{
    static const int noneEvent = 0;
    static const int readEvent = POLLIN | POLLPRI;
    static const int writeEvent = POLLOUT;
};

struct SelectableChannel::Impl
{
    // EventLoop* loop = nullptr;
    int fd = 0;
    int eventsInterested = EventMasks::noneEvent;
    int eventsReceived = EventMasks::noneEvent;
    bool handlingEvents = false;

    EventLoop* ownerLoop = nullptr;

    std::function<void()> onReadFunc;
    std::function<void()> onWriteFunc;
    std::function<void()> onCloseFunc;
    std::function<void()> onErrorFunc;
};

SelectableChannel::SelectableChannel(int fd)
{
    _pImpl.reset(new SelectableChannel::Impl());

    // in case that O_NONBLOCK is not set
    int fflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, fflags | O_NONBLOCK);
    _pImpl->fd = fd;
}

SelectableChannel::~SelectableChannel()
{
    auto loop = _pImpl->ownerLoop;
    if (loop) {
        loop->removeChannel(this);
    }
}

void
SelectableChannel::onRead(std::function<void()> callback)
{
    _pImpl->onReadFunc = callback;
}

void
SelectableChannel::onWrite(std::function<void()> callback)
{
    _pImpl->onWriteFunc = callback;
}

void
SelectableChannel::onClose(std::function<void()> callback)
{
    _pImpl->onCloseFunc = callback;
}

void
SelectableChannel::onError(std::function<void()> callback)
{
    _pImpl->onErrorFunc = callback;
}

void
SelectableChannel::enableReading()
{
    _pImpl->eventsInterested |= EventMasks::readEvent;
    if (_pImpl->ownerLoop) {
        _pImpl->ownerLoop->updateChannel(this);
    }
}

void
SelectableChannel::disableReading()
{
    _pImpl->eventsInterested &= EventMasks::noneEvent;
    if (_pImpl->ownerLoop) {
        _pImpl->ownerLoop->updateChannel(this);
    }
}

void
SelectableChannel::enableWriting()
{
    _pImpl->eventsInterested |= EventMasks::writeEvent;
    if (_pImpl->ownerLoop) {
        _pImpl->ownerLoop->updateChannel(this);
    }
}

void
SelectableChannel::disableWriting()
{
    _pImpl->eventsInterested &= ~(EventMasks::writeEvent);
    if (_pImpl->ownerLoop) {
        _pImpl->ownerLoop->updateChannel(this);
    }
}

void
SelectableChannel::disableAll()
{
    _pImpl->eventsInterested &= EventMasks::noneEvent;
    if (_pImpl->ownerLoop) {
        _pImpl->ownerLoop->updateChannel(this);
    }
}

void
SelectableChannel::setOwnerEventLoop(EventLoop* l)
{
    _pImpl->ownerLoop = l;
}

void
SelectableChannel::unregisterSelf()
{
    if (_pImpl->ownerLoop) {
        _pImpl->ownerLoop->removeChannel(this);
    }
}

int
SelectableChannel::eventsInterested()
{
    return _pImpl->eventsInterested;
}

void
SelectableChannel::setEventsReceived(int revt)
{
    _pImpl->eventsReceived = revt;
}

void
SelectableChannel::handleEventsReceived()
{
    _pImpl->handlingEvents = true;
    MUQUINETD_LOG(debug) << "SelectableChannel {fd = " << _pImpl->fd
                         << "} handling its events";

    int revents = _pImpl->eventsReceived;

    // TODO: log
    if ((revents & POLLHUP) && !(revents & POLLIN)) {
        if (_pImpl->onCloseFunc)
            _pImpl->onCloseFunc();
    }
    if (revents & (POLLERR | POLLNVAL)) {
        if (_pImpl->onErrorFunc)
            _pImpl->onErrorFunc();
    }
    if (revents & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (_pImpl->onReadFunc)
            _pImpl->onReadFunc();
    }
    if (revents & POLLOUT) {
        if (_pImpl->onWriteFunc)
            _pImpl->onWriteFunc();
    }

    _pImpl->handlingEvents = false;
}

int
SelectableChannel::fd()
{
    return _pImpl->fd;
}
