/*
    Copyright 2021 natinusala

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

#include <stdexcept>
#include <borealis/core/logger.hpp>

namespace brls
{
bool endsWith(const std::string& data, const std::string& suffix);

bool startsWith(const std::string& data, const std::string& prefix);
/**
 * Prints the given error message message and throws a std::logic_error.
 */
[[noreturn]] void fatal(std::string message);

std::string loadFileContents(const std::string& path);

/**
 * The whole of a regular file of at most `maxBytes` bytes, in `text`. False,
 * `text` untouched, for a file that is not there, is not a regular file (a
 * folder, a pipe, a device) or is larger.
 */
bool readSmallFile(const std::string& path, size_t maxBytes, std::string* text);

/**
 * True for a resource name ("@res/…") with no ".." part: one that stays in
 * the resources.
 */
bool isResourceNameSafe(const std::string& name);

/**
 * `text` with its control characters (ASCII and C1) written as `\xNN`, for a
 * log line or a message that quotes a file a user gave.
 */
std::string escapeControlChars(const std::string& text);

} // namespace brls
