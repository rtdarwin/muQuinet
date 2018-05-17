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

#ifndef MUQUINETD_MUQUINETD_H
#define MUQUINETD_MUQUINETD_H

#include <memory>

#include "muquinetd/base/Singleton.h"

class muQuinetd : public Singleton<muQuinetd>
{
    friend class Singleton<muQuinetd>;

public:
    enum class exit_status
    {
        SUCCESS = 0,
        FAILURE = 1,
        BAD_CMDLINE,
        BAD_CONF_FILE,
    };

public:
    void init(int argc, char* argv[]);
    void run();
    void stop();
    void exit(enum exit_status);

private:
    muQuinetd();
    ~muQuinetd();

    void readConf(int argc, char* argv[]);
    void initSignalActions();
    void initLogging();
    void initCommunication();

private:
    struct Impl;
    std::unique_ptr<Impl> _pImpl;
};

#endif // MUQUINETD_MUQUINETD_H
