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

#ifndef MUQUINETD_MUX_SELECTABLECHANNEL_H
#define MUQUINETD_MUX_SELECTABLECHANNEL_H

#include <functional>
#include <memory>

class EventLoop;

class SelectableChannel
{
public:
    SelectableChannel(int fd);
    ~SelectableChannel();
    // Non-copyable, Non-moveable
    SelectableChannel(const SelectableChannel&) = delete;
    SelectableChannel& operator=(const SelectableChannel&) = delete;
    SelectableChannel(SelectableChannel&&) = delete;
    SelectableChannel& operator=(SelectableChannel&&) = delete;

    void setOnReadCB(std::function<void()> callback);
    void setOnWriteCB(std::function<void()> callback);
    void setOnCloseCB(std::function<void()> callback);
    void setOnErrorCB(std::function<void()> callback);

    // For SelectableChannel creator use
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();

    // For who control the EventLoop use
    void setOwnerEventLoop(EventLoop*);
    void unregisterSelf();

    // For EventLoop.poller use
    int eventsInterested();
    void setEventsReceived(int revt);
    void handleEventsReceived();
    int fd();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_MUX_SELECTABLECHANNEL_H
