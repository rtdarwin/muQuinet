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

#ifndef MUQUINETD_LOGGING_H
#define MUQUINETD_LOGGING_H

#include <boost/log/attributes/attribute_cast.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <string>

////////////////////////////////////////////////////////////////////////////////
// Define Macro `MUQUINETD_LOG(sev)` for logging
//
// - use conjunction with BOOST_LOG_FUNCTION, BOOST_LOG_NAMED_SCOPE
//   to get source file/function specific attributes
//
// == EXAMPLE BEGIN
// BOOST_LOG_FUNCTION();
// BOOST_LOG_NAMED_SCOPE("case 0")
// MUQUINETD_LOG(info) << "a example log";
// MUQUINETD_LOG(debug) << "a example log";
// == EXAMPLE END

// clang-format off

#define MUQUINETD_LOG(sev)                                                     \
    BOOST_LOG_STREAM_WITH_PARAMS                                               \
    (                                                                          \
        (muQuinetd_logger::get()),                                             \
          (details::set_get_attrib("File", details::path_to_filename(__FILE__))) \
          (details::set_get_attrib("Line", __LINE__))                            \
          (::boost::log::keywords::severity = (::boost::log::trivial::sev))      \
    )

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT
(
    muQuinetd_logger
    , ::boost::log::sources::severity_logger_mt<::boost::log::trivial::severity_level>
)

// clang-format on
// Macro for muQuinetd logging
////////////////////////////////////////////////////////////////////////////////

class LoggingInitializer
{
public:
    LoggingInitializer() = default;
    // Rule of zero

    void run();

private:
};

class LoggingThreadInitializer
{
public:
    LoggingThreadInitializer() = default;
    // Rule of zero

    void run();
};

namespace details {

// Set attribute and return the new value
template <typename ValueType>
ValueType
set_get_attrib(const char* name, ValueType value)
{
    auto attr = boost::log::attribute_cast<
        boost::log::attributes::mutable_constant<ValueType>>(
        boost::log::core::get()->get_thread_attributes()[name]);
    attr.set(value);
    return attr.get();
}

// Convert file path to only the filename
std::string path_to_filename(std::string path);

} // namespace details {

#endif // MUQUINETD_LOGGING_H
