/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <WCDB/Console.hpp>
#include <WCDB/Error.hpp>
#include <WCDB/Notifier.hpp>
#include <iostream>

namespace WCDB {

Console* Console::shared()
{
    static Console* s_shared = new Console;
    return s_shared;
}

Console::Console() : m_debugable(false)
{
#if DEBUG
    setDebuggable(true);
#endif
    setLogger(Console::log);
}

void Console::setDebuggable(bool debuggable)
{
    m_debugable = debuggable;
    constexpr const char* breakpointIdentifier = "com.Tencent.WCDB.Notifier.Console.Breakpoint";
    if (m_debugable) {
        Notifier::shared()->setNotification(
        std::numeric_limits<int>::max(), breakpointIdentifier, Console::breakpoint);
    } else {
        Notifier::shared()->unsetNotification(breakpointIdentifier);
    }
}

bool Console::debuggable()
{
    return Console::shared()->isDebuggable();
}

bool Console::isDebuggable()
{
    return m_debugable;
}

void Console::setLogger(const Callback& callback)
{
    constexpr const char* logIdentifier = "com.Tencent.WCDB.Notifier.Console.Log";
    if (callback) {
        Notifier::shared()->setNotification(
        std::numeric_limits<int>::min(), logIdentifier, callback);
    } else {
        Notifier::shared()->unsetNotification(logIdentifier);
    }
}

void Console::log(const Error& error)
{
    switch (error.level) {
    case Error::Level::Ignore:
        break;
    case Error::Level::Debug:
        if (!debuggable()) {
            break;
        }
        // fallthrough
    case Error::Level::Warning:
    case Error::Level::Notice:
    case Error::Level::Error:
    case Error::Level::Fatal:
        print(error);
        break;
    }
}

void Console::print(const Error& error)
{
    std::ostringstream stream;
    stream << "[" << Error::levelName(error.level) << ": ";
    stream << (int) error.code();
    if (!error.message.empty()) {
        stream << ", " << error.message;
    }
    stream << "]";

    for (const auto& info : error.infos.getIntegers()) {
        stream << ", " << info.first << ": " << info.second;
    }
    for (const auto& info : error.infos.getStrings()) {
        if (!info.second.empty()) {
            stream << ", " << info.first << ": " << info.second;
        }
    }
    for (const auto& info : error.infos.getDoubles()) {
        stream << ", " << info.first << ": " << info.second;
    }
    stream << std::endl;
    std::cout << stream.str();
}

void Console::breakpoint(const Error& error)
{
    if (error.level == Error::Level::Fatal) {
        std::cout << "Set breakpoint at Console::breakpoint to debug\n";
        abort();
    }
}

void Console::fatal(const String& message, const char* file, int line)
{
    Error error;
    error.setCode(Error::Code::Misuse, "Assertion");
    error.level = Error::Level::Fatal;
    error.message = message;
    if (file) {
        error.infos.set("File", file);
    }
    error.infos.set("Line", line);
    Notifier::shared()->notify(error);
}

} // namespace WCDB
