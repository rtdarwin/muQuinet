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

#ifndef MUQUINETD_MUX_EVENTLOOP_H
#define MUQUINETD_MUX_EVENTLOOP_H

#include <memory>

class SelectableChannel;

/** Enhanced Reactor pattern (node.js)
 *
 * EventLoop 模块向外部提供两个概念：EventLoop, SelectableChannel
 *
 * 注意：
 *  - 我不喜欢 `SelectableChannel 能自己注册到 EventLoop 中' 这种自动化的功能，
 *     所以，muQuinetd 中所有 Channel 要想注册到某 EventLoop 需显式指定
 *
 *  - 但 SelectableChannel 销毁时具有自动从 EventLoop 解注册自己
 *     的自动化功能，也有在自己感兴趣的事件改变时自动通知 EventLoop
 *     的自动化功能
 *
 *  - EventLoop 不对 SelectableChannel 生命周期负责
 */
class EventLoop
{
public:
    EventLoop();
    ~EventLoop();
    // Non-copyable, Non-moveable
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    void addChannel(SelectableChannel*);
    void removeChannel(SelectableChannel*);
    void updateChannel(SelectableChannel*);
    bool hasChannel(SelectableChannel*);

    void loop();
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif
