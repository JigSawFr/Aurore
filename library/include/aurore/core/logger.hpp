/*
    Copyright 2019 natinusala
    Copyright 2019 p-sam

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#ifdef ANDROID
#include <android/log.h>
#endif

#ifdef __PSV__
#include <psp2/kernel/clib.h>
#endif

#ifdef PS4
#include <orbis/libkernel.h>
#include <aurore/platforms/ps4/ps4_sysmodule.hpp>
#endif

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <ctime>
#include <mutex>
#include <string>
#include <aurore/core/event.hpp>
#include <aurore/core/time.hpp>

namespace aurore
{

enum class LogLevel
{
    LOG_ERROR = 0,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_VERBOSE
};

#ifdef IOS
#define AURORE_ERROR_COLOR "🔴"
#define AURORE_WARNING_COLOR "🟠"
#define AURORE_INFO_COLOR "🔵"
#define AURORE_DEBUG_COLOR "🟢"
#define AURORE_VERBOSE_COLOR "⚪️"
#else
#define AURORE_ERROR_COLOR "[0;31m"
#define AURORE_WARNING_COLOR "[0;33m"
#define AURORE_INFO_COLOR "[0;34m"
#define AURORE_DEBUG_COLOR "[0;32m"
#define AURORE_VERBOSE_COLOR "[0;37m"
#endif

#define AURORE_LOG_ERROR(format, ...) aurore::Logger::error("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define AURORE_LOG_WARNING(format, ...) aurore::Logger::warning("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define AURORE_LOG_INFO(format, ...) aurore::Logger::info("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define AURORE_LOG_DEBUG(format, ...) aurore::Logger::debug("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define AURORE_LOG_VERBOSE(format, ...) aurore::Logger::verbose("{}:{} " format, __FILE__, __LINE__, ##__VA_ARGS__)

class Logger
{
  public:
    using TimePoint = std::chrono::system_clock::time_point;

    static void setLogLevel(LogLevel logLevel);

    static LogLevel getLogLevel();

    static void setLogOutput(std::FILE* logOut);

    /**
     * If sets to true, each log operation will lock a mutex, making the Logger thread-safe.
     */
    static void setThreadSafeLogging(bool threadSafeLogging);

    template <typename... Args>
    inline static void log(LogLevel level, std::string prefix, std::string color, fmt::format_string<Args...> format, Args&&... args)
    {
        if (Logger::logLevel < level)
            return;

        TimePoint now = std::chrono::system_clock::now();
        uint64_t ms   = std::chrono::duration_cast<std::chrono::milliseconds>(
                          now.time_since_epoch())
                          .count()
            % 1000;
#ifdef PS4
        OrbisDateTime lt {};
        if (sceRtcGetCurrentClockLocalTime)
            sceRtcGetCurrentClockLocalTime(&lt);
#else
        std::time_t tt  = std::chrono::system_clock::to_time_t(now);
        std::tm time_tm = localTime(tt);
#endif
        std::unique_lock<std::mutex> lock;
        if (Logger::threadSafeLogging)
            lock = std::unique_lock { logMtx };

        std::FILE* out = resolveLogOutput();

        try
        {
            // In C++17 fmt checks the format string when the line is
            // written, not when it is compiled: a wrong one throws here,
            // caught below rather than ending the program.
            std::string log = fmt::format(format, std::forward<Args>(args)...);

#ifdef IOS
            if (out)
                fmt::print(out, "{:%H:%M:%S}.{:03d} {} {}\n", time_tm, (int)ms, color, log);
#elif defined(ANDROID)
            __android_log_print(6 - (int)level, "aurore", "%02d:%02d:%02d.%03d %s\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, (int)ms, log.c_str());
#elif defined(__PSV__)
            sceClibPrintf("%02d:%02d:%02d.%03d\033%s[%s]\033[0m %s\n", time_tm.tm_hour, time_tm.tm_min, time_tm.tm_sec, (int)ms, color.c_str(), prefix.c_str(), log.c_str());
#elif defined(PS4)
            sceKernelDebugOutText(0, fmt::format("{:02d}:{:02d}:{:02d}.{:03d}\033{}[{}]\033[0m {}\n", lt.hour, lt.minute, lt.second, (int)ms, color, prefix, log).c_str());
#else
            if (out)
                fmt::print(out, "{:%H:%M:%S}.{:03d}\033{}[{}]\033[0m {}\n", time_tm, (int)ms, color, prefix, log);
#endif

            logEvent.fire(now, level, log);
        }
        catch (const std::exception& e)
        {
            // will be printed after the first fmt::print (so after the log tag)
            printf("! Invalid log format string: \"%s\": %s\n", format.get().data(), e.what());
        }

        // Flush on every platform: once the output is not a terminal (nxlink
        // socket, file, pipe) stdio buffers it fully, and the lines a crash
        // leaves in the buffer are the ones needed to diagnose it.
        if (out)
            fflush(out);
    }

    template <typename... Args>
    inline static void error(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_ERROR, "ERROR", AURORE_ERROR_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void warning(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_WARNING, "WARNING", AURORE_WARNING_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void info(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_INFO, "INFO", AURORE_INFO_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void debug(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_DEBUG, "DEBUG", AURORE_DEBUG_COLOR, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline static void verbose(fmt::format_string<Args...> format, Args&&... args)
    {
        Logger::log(LogLevel::LOG_VERBOSE, "VERBOSE", AURORE_VERBOSE_COLOR, format, std::forward<Args>(args)...);
    }

    static Event<TimePoint, LogLevel, std::string>* getLogEvent()
    {
        return &logEvent;
    }

  private:
    inline static std::mutex logMtx;
    inline static bool threadSafeLogging = true;
    inline static Event<TimePoint, LogLevel, std::string> logEvent;
    // Resolved on first use, not by a static initialiser: on newlib
    // (devkitPro) stdout can still be null when statics are initialised.
    static std::FILE* resolveLogOutput()
    {
        if (!logOutSet)
        {
            logOut    = stdout;
            logOutSet = true;
        }
        return logOut;
    }

    inline static std::FILE* logOut = nullptr;
    inline static bool logOutSet    = false;
    inline static LogLevel logLevel = LogLevel::LOG_INFO;
};

} // namespace aurore
