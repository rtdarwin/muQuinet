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

#include "muQuinetd.h"

#include <bits/exception.h>
#include <cstdlib>
#include <iostream>

#include "muquinetd/ConfReader.h"
#include "muquinetd/Interface.h"
#include "muquinetd/Ip.h"
#include "muquinetd/Logging.h"
#include "muquinetd/Mux.h"
#include "muquinetd/Tcp.h"
#include "muquinetd/Udp.h"

using std::cout;
using std::exception;

struct muQuinetd::Impl
{
    bool running = false;
};

muQuinetd::muQuinetd()
{
    _pImpl.reset(new muQuinetd::Impl);
}

muQuinetd::~muQuinetd() = default;

void
muQuinetd::init(int argc, char* argv[])
{
    readConf(argc, argv);

    initSignalActions();
    initLogging();

    Interface::get()->init();
    Ip::get()->init();
    Tcp::get()->init();
    Udp::get()->init();
    Mux::get()->init();
}

void
muQuinetd::run()
{
    _pImpl->running = true;

    Interface::get()->start();
    Ip::get()->start();
    Tcp::get()->start();
    Udp::get()->start();
    Mux::get()->start();

    // FIXME: this thread will wait for user signals
    pthread_exit(NULL);
}

void
muQuinetd::stop()
{
    Interface::get()->stop();
    Ip::get()->stop();
    Tcp::get()->stop();
    Udp::get()->stop();
    Mux::get()->stop();

    _pImpl->running = false;
}

void
muQuinetd::exit(enum exit_status e)
{
    if (_pImpl->running) {
        this->stop();
    }

    switch (e) {
        case exit_status::SUCCESS:
            ::exit(EXIT_SUCCESS);
        default:
            ::exit(EXIT_FAILURE);
    };
}

void
muQuinetd::readConf(int argc, char* argv[])
{
    try {
        ConfReader reader;
        reader.readCmdLine(argc, argv);
        reader.readConfFile();
    } catch (const exception& e) {
        cout << "Error in Command line syntax"
             << "\n";
        cout << "  " << e.what() << "\n";
        this->exit(exit_status::FAILURE);
    }
}

void
muQuinetd::initSignalActions()
{
    // FIXME
    // SIGINT  - close gracefully
}

void
muQuinetd::initLogging()
{
    try {
        /*  1. global */
        LoggingInitializer i;
        i.run();

        /*  2. this thread */
        LoggingThreadInitializer ti;
        ti.run();
    } catch (const exception& e) {
        cout << "Error when starting logging facility"
             << "\n";
        cout << "  " << e.what() << "\n";
        this->exit(exit_status::FAILURE);
    }
}
