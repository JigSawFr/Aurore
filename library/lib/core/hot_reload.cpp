// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// Hot reload — the XML and theme files of the screen shown, watched on a
// desktop, and the activity built again when one changes.
#include <algorithm>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <vector>
#include <aurore/core/application.hpp>
#include <aurore/core/hot_reload.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/theme.hpp>
#include <aurore/core/time.hpp>
#include <aurore/core/util.hpp>
#include <aurore/core/view.hpp>
#ifdef USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#elif __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#elif __has_include("experimental/filesystem")
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#error "Failed to include <filesystem> header!"
#endif

#if defined(__SWITCH__) || defined(__PSV__) || defined(PS4) || defined(ANDROID) || defined(IOS) || defined(TVOS) \
    || defined(__WINRT__)
#define AURORE_NO_HOT_RELOAD
#endif

namespace aurore
{

// Named: in a unity build, the other files' helpers do not meet these
namespace hot_reload_detail
{
    // Twice a second or so: an editor's save is seen at the next look, and
    // acted upon at the one after, once the file has stopped changing
    constexpr Time POLL_INTERVAL = 300000;
    // A file watched larger than this is not read (an XML or a theme is a
    // few kilobytes)
    constexpr size_t MAX_FILE_BYTES = 4 * 1024 * 1024;
    using FileTime                  = decltype(fs::last_write_time(fs::path()));

    struct Watched
    {
        std::string path;
        bool theme         = false;
        bool read          = false; // its contents were read at least once
        size_t hash        = 0;
        bool pending       = false;
        size_t pendingHash = 0;
        // When it was last read, and its size: read again only when they change
        FileTime modified {};
        uintmax_t size = 0;
        std::set<Activity*> activities; // the ones built from it (XML)
    };

    std::vector<Watched> files;
    std::string resources;
    Time lastPoll = 0;

    bool contents(const std::string& path, size_t* hash)
    {
        std::string text;
        if (!readSmallFile(path, MAX_FILE_BYTES, &text))
            return false;
        *hash = std::hash<std::string> {}(text);
        return true;
    }

    // Its time and size now; false when it is not there
    bool stamp(const std::string& path, FileTime* modified, uintmax_t* size)
    {
        try
        {
            *modified = fs::last_write_time(path);
            *size     = fs::file_size(path);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    Watched& watch(const std::string& path)
    {
        for (Watched& file : files)
            if (file.path == path)
                return file;
        Watched file;
        file.path = path;
        stamp(path, &file.modified, &file.size);
        file.read = contents(path, &file.hash);
        files.push_back(file);
        Logger::debug("hot reload: watching {}", escapeControlChars(path));
        return files.back();
    }
} // namespace hot_reload_detail

using namespace hot_reload_detail;

void HotReload::setEnabled(bool on, const std::string& resourcesDir)
{
#ifdef AURORE_NO_HOT_RELOAD
    if (on)
        Logger::warning("hot reload: not on this platform, its resources are not read from a folder");
    return;
#else
    HotReload::enabled = on;
    resources          = resourcesDir;
    if (!resources.empty() && resources.back() != '/' && resources.back() != '\\')
        resources += '/';
    if (!on)
        files.clear();
    else
        Logger::info("hot reload: on{}", resources.empty() ? "" : ", resources read from " + resources);
#endif
}

std::string HotReload::diskResource(const std::string& value)
{
    if (!HotReload::enabled || resources.empty() || value.rfind("@res/", 0) != 0 || !isResourceNameSafe(value))
        return "";
    std::string path = resources + value.substr(5);
    if (!std::ifstream(path).good())
        return "";
    return path;
}

void HotReload::noteXMLFile(const std::string& path)
{
    if (!HotReload::enabled)
        return;
    Watched& file      = watch(path);
    Activity* activity = HotReload::building;
    if (!activity)
    {
        std::vector<Activity*> stack = Application::getActivitiesStack();
        if (!stack.empty())
            activity = stack.back();
    }
    if (activity)
        file.activities.insert(activity);
}

void HotReload::setThemeFiles(const std::vector<std::string>& paths)
{
    if (!HotReload::enabled)
        return;
    files.erase(std::remove_if(files.begin(), files.end(), [](const Watched& file)
                    { return file.theme; }),
        files.end());
    for (const std::string& path : paths)
    {
        // A preset has no file; a resource is one when read from a folder
        std::string disk = path;
        if (path.starts_with("builtin:"))
            continue;
        if (path.starts_with("@res/"))
        {
            disk = HotReload::diskResource(path);
            if (disk.empty())
            {
#ifndef USE_LIBROMFS
                disk = std::string(AURORE_RESOURCES) + path.substr(5);
#else
                continue;
#endif
            }
        }
        watch(disk).theme = true;
    }
}

void HotReload::forgetActivity(Activity* activity)
{
    for (Watched& file : files)
        file.activities.erase(activity);
}

void HotReload::poll()
{
    if (!HotReload::enabled)
        return;
    Time now = getCPUTimeUsec();
    if (now - lastPoll < POLL_INTERVAL)
        return;
    lastPoll = now;

    std::vector<Activity*> stack = Application::getActivitiesStack();
    Activity* top                = stack.empty() ? nullptr : stack.back();

    bool rebuild = false;
    bool theme   = false;
    for (Watched& file : files)
    {
        // Unchanged since it was read: not read again
        FileTime modified;
        uintmax_t size;
        if (!stamp(file.path, &modified, &size)) // being written, or moved away
            continue;
        if (file.read && !file.pending && modified == file.modified && size == file.size)
            continue;
        file.modified = modified;
        file.size     = size;

        size_t hash;
        if (!contents(file.path, &hash))
            continue;
        if (file.read && hash == file.hash)
        {
            file.pending = false;
            continue;
        }
        if (!file.pending || file.pendingHash != hash)
        {
            file.pending     = true;
            file.pendingHash = hash;
            continue;
        }

        file.pending = false;
        file.read    = true;
        file.hash    = hash;
        Logger::info("hot reload: {} changed", escapeControlChars(file.path));
        if (file.theme)
        {
            theme   = true;
            rebuild = true; // metrics are read when the views are made
            continue;
        }
        View::forgetXMLCache(file.path);
        if (file.activities.contains(top))
            rebuild = true;
        else
            Logger::info("hot reload: {} is not used by the activity on top, it shows next time it is made", file.path);
    }

    // Again from the file in use, which replaces the files watched
    // (A file that cannot be read keeps the theme as it was; loadFile logs why.)
    if (theme && !Theme::getCurrentPath().empty())
        (void)Theme::loadFile(Theme::getCurrentPath());

    if (rebuild && top)
        Application::rebuildActivity();
}

} // namespace aurore
