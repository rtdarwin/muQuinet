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

#ifndef MUQUINETD_BASE_MUTEXLOCK_H
#define MUQUINETD_BASE_MUTEXLOCK_H

#include <pthread.h>

class MutexLock
{
public:
    MutexLock();
    ~MutexLock();
    // Non-copyable, Non-moveable
    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;
    MutexLock(MutexLock&&) = delete;
    MutexLock& operator=(MutexLock&&) = delete;

    void lock();
    void tryLock();
    void unlock();
    bool isLockedByThisThread();

private:
    pthread_mutex_t _mutex;
    pthread_t _holder;
};

/** MutexLockGuard object can automatically release mutex at it's end of
 *  lifetime
 *
 * Especially useful when an expection occur.
 */
class MutexLockGuard
{
public:
    MutexLockGuard(MutexLock&);
    ~MutexLockGuard();

private:
    MutexLock& _mutex;
};

#endif // MUQUINETD_BASE_MUTEXLOCK_H
