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

#include "muquinetd/base/MutexLock.h"

MutexLock::MutexLock()
    : _holder(0)
{
    pthread_mutex_init(&_mutex, NULL);
}

MutexLock::~MutexLock()
{
    pthread_mutex_destroy(&_mutex);
}

void
MutexLock::lock()
{
    pthread_mutex_lock(&_mutex);
}

void
MutexLock::tryLock()
{
    pthread_mutex_trylock(&_mutex);
}

void
MutexLock::unlock()
{
    pthread_mutex_unlock(&_mutex);
}

bool
MutexLock::isLockedByThisThread()
{
    return pthread_self() == _holder;
}

MutexLockGuard::MutexLockGuard(MutexLock& lock)
    : _mutex(lock)
{
    _mutex.lock();
}

MutexLockGuard::~MutexLockGuard()
{
    _mutex.unlock();
}
