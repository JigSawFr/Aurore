// SPDX-FileCopyrightText: 2019 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <format>

#include <aurore/core/logger.hpp>

namespace aurore
{

void Logger::setLogLevel(LogLevel newLogLevel)
{
    Logger::logLevel = newLogLevel;
}

LogLevel Logger::getLogLevel()
{
    return Logger::logLevel;
}

void Logger::setLogOutput(std::FILE* newLogOut)
{
    std::lock_guard<std::mutex> lock(logMtx);
    Logger::logOut    = newLogOut;
    Logger::logOutSet = true;
}

void Logger::setThreadSafeLogging(bool newThreadSafeLogging)
{
    Logger::threadSafeLogging = newThreadSafeLogging;
}

} // namespace aurore
