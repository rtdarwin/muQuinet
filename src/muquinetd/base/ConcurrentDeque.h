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

#ifndef MUQUINETD_BASE_CONCURRENTDEQUEUE_H
#define MUQUINETD_BASE_CONCURRENTDEQUEUE_H

#include <deque>

#include "muquinetd/base/MutexLock.h"

template <typename T>
class ConcurrentDeque
{
public:
    typedef T value_type;

    ConcurrentDeque() = default;
    // Non-copyable, but Moveable
    ConcurrentDeque(const ConcurrentDeque&) = delete;
    ConcurrentDeque& operator=(const ConcurrentDeque&) = delete;
    ConcurrentDeque(ConcurrentDeque&&) = default;
    ConcurrentDeque& operator=(ConcurrentDeque&&) = default;

    // Element access
    T& front();
    T& back();

    // Capacity
    bool empty();
    size_t size();

    // Modifiers
    void clear();
    void push_back(const T& value);
    void push_back(T&& value);
    void pop_back();
    void push_front(const T& value);
    void push_front(T&& value);
    void pop_front();
    void resize(size_t count);
    void resize(size_t count, const value_type& value);
    void swap(ConcurrentDeque& other);

private:
    MutexLock _mutex;
    std::deque<T> _q;
};

#ifdef SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0
#undef SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0
#endif
#define SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0(retT, func)                     \
    template <typename T>                                                      \
    retT ConcurrentDeque<T>::func()                                            \
    {                                                                          \
        MutexLockGuard guard(_mutex);                                          \
        return _q.func();                                                      \
    }

#ifdef SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1
#undef SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1
#endif
#define SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1(retT, func, argT)               \
    template <typename T>                                                      \
    retT ConcurrentDeque<T>::func(argT value)                                  \
    {                                                                          \
        MutexLockGuard guard(_mutex);                                          \
        return _q.func(value);                                                 \
    }

#ifdef SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_2
#undef SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_2
#endif
#define SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_2(retT, func, arg1T, arg2T)       \
    template <typename T>                                                      \
    retT ConcurrentDeque<T>::func(arg1T arg1, arg2T arg2)                      \
    {                                                                          \
        MutexLockGuard guard(_mutex);                                          \
        return _q.func(arg1, arg2);                                            \
    }

SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0(T&, front)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0(T&, back)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0(bool, empty)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0(size_t, size)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0(void, clear)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1(void, push_back, const T&)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1(void, push_back, T&&)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_0(void, pop_front)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1(void, push_front, const T&)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1(void, push_front, T&&)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_1(void, resize, size_t)
SYNTHESIS_CONCURRENTDEQUE_TEMPL_IMPL_2(void, resize, size_t, const value_type&)

#endif // MUQUINETD_BASE_CONCURRENTDEQUEUE_H
