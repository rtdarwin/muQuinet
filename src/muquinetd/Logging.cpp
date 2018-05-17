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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "Logging.h"

#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/value_ref.hpp>
#include <boost/phoenix/bind.hpp>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <sys/syscall.h>

#include "Conf.h"

namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace phoenix = boost::phoenix;

namespace {

/* Define place holder attributes */
BOOST_LOG_ATTRIBUTE_KEYWORD(process_id, "ProcessID",
                            attrs::current_process_id::value_type)
BOOST_LOG_ATTRIBUTE_KEYWORD(filename, "File", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(lineno, "Line", int)

// Get Process native ID
attrs::current_process_id::value_type::native_type
get_native_process_id(logging::value_ref<attrs::current_process_id::value_type,
                                         tag::process_id> const& pid)
{
    if (pid)
        return pid->native_id();
    return 0;
}

std::string
file_lineno_with_padding(
    logging::value_ref<std::string, tag::filename> const& file,
    logging::value_ref<int, tag::lineno> const& line)
{
    // @in
    const string& filename = file.get<std::string>();
    int lineno = line.get<int>();

    // @out
    constexpr int maxlen =
        18 + 1 + 4; // filename(at most 18) + :(1) + lineno(at most 4)
    // snprintf 会多写一个 NULL 到字符串末尾，此处加的 1 字节
    // 是给 snprintf 预留专门放 NULL 的，不然越界了就要 segv...
    char padded[maxlen + 1];
    memset(padded, ' ', maxlen);

    int i = 0;

    // file
    {
        int nbytes_to_copy = std::min((int)filename.length(), 18);
        memcpy(padded, filename.c_str(), nbytes_to_copy);
        i += nbytes_to_copy;
    }

    // :lineno
    {
        i += snprintf(padded + i, 1 + 4 + 1, ":%d", lineno);
        padded[i] = ' '; // discard NULL written by snprintf
    }

    constexpr int fixed_minlen = 17;
    int len = i > fixed_minlen ? i : fixed_minlen;
    return std::string(padded, len);
}

} // namespace {

namespace details {

std::string
path_to_filename(std::string path)
{
    return path.substr(path.find_last_of("/\\") + 1);
}

} // namespace details {

void
LoggingInitializer::run()
{
    bool logging2stdout = Conf::get()->logging.to_stdout;

    /*  1. logging attributes */

    auto core = logging::core::get();
    // LineID, TimeStamp, ProcessID, ThreadID @ global
    logging::add_common_attributes();
    core->add_global_attribute("Scope", attrs::named_scope());
    core->add_thread_attribute("File",
                               attrs::mutable_constant<std::string>(""));
    core->add_thread_attribute("Line", attrs::mutable_constant<int>(0));
    core->add_thread_attribute("NativeThreadID",
                               attrs::constant<long>(syscall(SYS_gettid)));
    /*  2. logging level */

    logging::trivial::severity_level l;
    const char* lstr = Conf::get()->logging.level.c_str();
    // debug, info, warning, error, fatal
    // each has different first character in their spelling
    switch (*lstr) {
        case 'd':
            l = logging::trivial::debug;
            break;
        case 'i':
            l = logging::trivial::info;
            break;
        case 'w':
            l = logging::trivial::warning;
            break;
        case 'e':
            l = logging::trivial::error;
            break;
        case 'f':
            l = logging::trivial::fatal;
            break;
        default:
            throw std::invalid_argument("Invalid logging level");
    }

    logging::core::get()->set_filter(logging::trivial::severity >= l);

    /*  3. logging format */

    // clang-format off

    // Boost.Format style
    //
    //  LineId [TimeStamp] [ProcessID  ThreadID] [File:Line] <severity> message
    const auto& log_format =
        expr::format("%1% [%2%] [P%3% T%4%] [%5%] <%6%> %7%")
            % expr::attr<unsigned int>("LineID")
            % expr::format_date_time<boost::posix_time::ptime>
                    // ("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                    ("TimeStamp", "%Y-%m-%d %H:%M:%S")
            % boost::phoenix::bind(&get_native_process_id, process_id.or_none())
            % expr::attr<long>("NativeThreadID")
            % boost::phoenix::bind(&file_lineno_with_padding
                                   , filename.or_none()
                                   , lineno.or_none())
            // % expr::attr<attrs::named_scope::value_type>("Scope")
            % logging::trivial::severity
            % expr::smessage;

    /*  4. logging sink */

    if (logging2stdout) {
        logging::add_console_log
        (
             std::cout,
             keywords::format = log_format
        );
    } else {
        logging::add_file_log
        (
             keywords::file_name = "muquinetd.%N.log",
             keywords::rotation_size = 10 * 1014 * 1024,
             keywords::time_based_rotation =
                 sinks::file::rotation_at_time_point(0, 0, 0),
             keywords::format = log_format
        );
    }

    // clang-format on
}

void
LoggingThreadInitializer::run()
{
    auto core = logging::core::get();
    core->add_thread_attribute("File",
                               attrs::mutable_constant<std::string>(""));
    core->add_thread_attribute("Line", attrs::mutable_constant<int>(0));
    core->add_thread_attribute("NativeThreadID",
                               attrs::constant<long>(syscall(SYS_gettid)));
}
