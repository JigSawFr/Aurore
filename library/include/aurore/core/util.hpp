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
#include <aurore/core/logger.hpp>

namespace aurore
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

// legacy-names: begin
/**
 * The environment variable AURORE_<name>, else the names borealis gave it:
 * BRLS_<name>, then BOREALIS_<name> (deprecated, logged once). nullptr when
 * none is set.
 */
const char* environmentVariable(const std::string& name);

/**
 * True for a theme colour or style metric key with borealis' prefix
 * ("brls/text"), deprecated: the same as "aurore/text".
 */
bool isLegacyKey(const std::string& key);

/// `key` with "aurore/" for borealis' "brls/"; any other key as it is.
std::string canonicalKey(const std::string& key);

/**
 * An XML tag with "aurore:" for borealis' "brls:" (deprecated, still read):
 * <brls:Box> is <aurore:Box>.
 */
std::string canonicalXMLName(const std::string& name);
// legacy-names: end

} // namespace aurore
