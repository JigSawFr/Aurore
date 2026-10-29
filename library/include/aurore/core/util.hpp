// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

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

/// The environment variable AURORE_<name>: nullptr when it is not set.
const char* environmentVariable(const std::string& name);

} // namespace aurore
