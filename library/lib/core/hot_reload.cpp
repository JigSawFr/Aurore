// Hot reload — the XML and theme files of the screen shown, watched on a
// desktop, and the activity built again when one changes.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <algorithm>
#include <borealis/core/application.hpp>
#include <borealis/core/hot_reload.hpp>
#include <borealis/core/logger.hpp>
#include <borealis/core/theme.hpp>
#include <borealis/core/time.hpp>
#include <borealis/core/view.hpp>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <vector>

#if defined(__SWITCH__) || defined(__PSV__) || defined(PS4) || defined(ANDROID) || defined(IOS) || defined(TVOS) \
    || defined(__WINRT__)
#define BRLS_NO_HOT_RELOAD
#endif

namespace brls
{

// Named: in a unity build, the other files' helpers do not meet these
namespace hot_reload_detail
{
    // Twice a second or so: an editor's save is seen at the next look, and
    // acted upon at the one after, once the file has stopped changing
    constexpr Time POLL_INTERVAL = 300000;

    struct Watched
    {
        std::string path;
        bool theme  = false;
        bool read   = false; // its contents were read at least once
        size_t hash = 0;
        bool pending       = false;
        size_t pendingHash = 0;
        std::set<Activity*> activities; // the ones built from it (XML)
    };

    std::vector<Watched> files;
    std::string resources;
    Time lastPoll = 0;

    bool contents(const std::string& path, size_t* hash)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return false;
        std::stringstream text;
        text << file.rdbuf();
        *hash = std::hash<std::string> {}(text.str());
        return true;
    }

    Watched& watch(const std::string& path)
    {
        for (Watched& file : files)
            if (file.path == path)
                return file;
        Watched file;
        file.path = path;
        file.read = contents(path, &file.hash);
        files.push_back(file);
        Logger::debug("hot reload: watching {}", path);
        return files.back();
    }
} // namespace hot_reload_detail

using namespace hot_reload_detail;

void HotReload::setEnabled(bool on, const std::string& resourcesDir)
{
#ifdef BRLS_NO_HOT_RELOAD
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
    if (!HotReload::enabled || resources.empty() || value.rfind("@res/", 0) != 0)
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
    Watched& file = watch(path);
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

void HotReload::noteThemeFile(const std::string& path)
{
    if (!HotReload::enabled)
        return;
    // Only the theme in use: one picked since replaces it
    files.erase(std::remove_if(files.begin(), files.end(), [&path](const Watched& file) { return file.theme && file.path != path; }),
        files.end());
    watch(path).theme = true;
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
    std::vector<std::string> themes;
    for (Watched& file : files)
    {
        // Theme::reset() went back to the colours of the code
        if (file.theme && file.path != Theme::getCurrentPath())
            continue;
        size_t hash;
        if (!contents(file.path, &hash)) // being written, or moved away
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
        Logger::info("hot reload: {} changed", file.path);
        if (file.theme)
        {
            themes.push_back(file.path);
            rebuild = true; // metrics are read when the views are made
            continue;
        }
        View::forgetXMLCache(file.path);
        if (file.activities.count(top))
            rebuild = true;
        else
            Logger::info("hot reload: {} is not used by the activity on top, it shows next time it is made", file.path);
    }

    for (const std::string& path : themes)
        Theme::loadFile(path);

    if (rebuild && top)
        Application::rebuildActivity();
    if (!themes.empty())
        Application::requestRedraw();
}

} // namespace brls
