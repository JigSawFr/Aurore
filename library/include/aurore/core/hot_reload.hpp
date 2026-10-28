// Hot reload — the XML and theme files of the screen shown, watched on a
// desktop, and the activity built again when one changes.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#pragma once

#include <string>
#include <vector>

namespace aurore
{

class Activity;

/**
 * What Application::setHotReload() turns on (docs/hot_reload.md); the core
 * calls the rest.
 */
class HotReload
{
  public:
    static void setEnabled(bool enabled, const std::string& resourcesDir = "");
    static bool isEnabled() { return enabled; }

    /**
     * The file on disk to read for an "@res/…" value, when hot reload reads
     * the resources from a folder of its own and the file is there; empty
     * otherwise.
     */
    static std::string diskResource(const std::string& value);

    // An XML file read from disk
    static void noteXMLFile(const std::string& path);

    // The theme file in use (Theme::loadFile) and the files it includes and
    // inherits, replacing the ones before; empty without a theme file. A
    // change to one of them reads the theme file in use again.
    static void setThemeFiles(const std::vector<std::string>& paths);

    // The activity whose content is being made, to which the XML files read
    // are counted (the activity on top otherwise)
    inline static Activity* building = nullptr;
    static void forgetActivity(Activity* activity);

    // From the main loop: looks at the files now and then
    static void poll();

  private:
    inline static bool enabled = false;
};

} // namespace aurore
