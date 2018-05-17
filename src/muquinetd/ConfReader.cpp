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

#include "ConfReader.h"

#include <bits/exception.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "Conf.h"
#include "muQuinetd.h"
#include "muquinet.h"

namespace po = boost::program_options;
using std::exception;
using std::cout;
using std::string;

void
ConfReader::readCmdLine(int argc, const char* const argv[])
{
    po::options_description desc("Options");

    // clang-format off
        desc.add_options()
          ("help,h", "show this help")
          ("version,V", "print version string")
          ("verbose,v", "print startup message more verbose")
          ("config,c", po::value<string>(),
             "specify configuration file")
          ("logging-level,l", po::value<string>(),
             "set log level (debug, info, warning, error, fatal)")
          ("logging-dir,L", po::value<string>(),
             "set logging file directory")
          ("logging-to-stdout,T",
             "whether to print logs to stdout (false on default"
             ". If present, logging to file will be disabled")
          ("tundev-name", po::value<string>())
          ("tundev-addr", po::value<string>())
          ("stack-addr", po::value<string>())
          ;
    // clang-format on

    /*  1. parse cmd line options */

    po::variables_map options;
    po::store(po::parse_command_line(argc, argv, desc), options);
    po::notify(options);

    /*  2. react to cmd line options */

    if (options.count("help")) {
        cout << "Usage: " << argv[0] << " [options]"
             << "\n";
        cout << desc << "\n";
        muQuinetd::get()->exit(muQuinetd::exit_status::SUCCESS);
    }
    //
    if (options.count("version")) {
        cout << "muQuinetd  version " << muquinet_version() << "\n";
        muQuinetd::get()->exit(muQuinetd::exit_status::SUCCESS);
    }
    //
    if (options.count("logging-level")) {
        string ll = options["logging-level"].as<string>();
        Conf::get()->logging.level = ll;
    }
    //
    if (options.count("logging-to-stdout")) {
        Conf::get()->logging.to_stdout = true;
    }
    //
    if (options.count("tundev-name")) {
        const string& name = options["tundev-name"].as<string>();
        Conf::get()->tundev.name = name;
    }
    //
    if (options.count("tundev-addr")) {
        const string& addr = options["tundev-addr"].as<string>();
        Conf::get()->tundev.addr_cidr = addr;
    }
    //
    if (options.count("stack-addr")) {
        const string& addr = options["stack-addr"].as<string>();
        Conf::get()->stack.addr = addr;
    }
}

void
ConfReader::readConfFile()
{
    // TODO
}
