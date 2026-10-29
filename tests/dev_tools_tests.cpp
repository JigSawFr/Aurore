/*
    Copyright 2026 JigSawFr

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

// The debugging aids' environment variables are read only in a build with
// AURORE_DEV_TOOLS (not a release one, by default); their functions work in
// every build

#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/core/application.hpp>
#include <aurore/core/hot_reload.hpp>
#include <aurore/core/inspector.hpp>
#include <aurore/core/profiler.hpp>
#include <aurore/test/harness.hpp>

#include <cstdlib>

using aurore::test::Harness;

#ifdef AURORE_DEV_TOOLS
static const bool BUILT_IN = true;
#else
static const bool BUILT_IN = false;
#endif

AURORE_TEST(environment_turns_the_debugging_aids_on_only_when_built_in)
{
    setenv("AURORE_INSPECTOR", "1", 1);
    setenv("AURORE_HOT_RELOAD", "1", 1);
    setenv("AURORE_REDRAW_DEBUG", "1", 1);
    setenv("AURORE_PROFILE", "1", 1);
    setenv("AURORE_STILL_FOCUS", "1", 1);
    setenv("AURORE_SAFE_AREA", "10,20", 1);

    Harness harness;
    harness.step(2);
    AURORE_CHECK_EQ(aurore::Inspector::isEnabled(), BUILT_IN);
    AURORE_CHECK_EQ(aurore::HotReload::isEnabled(), BUILT_IN);
    AURORE_CHECK_EQ(aurore::Application::isRedrawDebug(), BUILT_IN);
    AURORE_CHECK_EQ(aurore::Profiler::isEnabled(), BUILT_IN);
    AURORE_CHECK_EQ(ui::env_flag("STILL_FOCUS"), BUILT_IN);
    AURORE_CHECK_EQ(aurore::Application::getSafeArea() == (aurore::Insets { 10, 20, 10, 20 }), BUILT_IN);
}

AURORE_TEST(the_debugging_aids_functions_work_in_every_build)
{
    Harness harness;
    aurore::Application::setInspectorEnabled(true);
    AURORE_CHECK(aurore::Inspector::isEnabled());
    aurore::Application::setInspectorEnabled(false);
    aurore::Profiler::setEnabled(true);
    AURORE_CHECK(aurore::Profiler::isEnabled());
    aurore::Profiler::setEnabled(false);
    aurore::Application::setRedrawDebug(true);
    AURORE_CHECK(aurore::Application::isRedrawDebug());
    aurore::Application::setRedrawDebug(false);
    const aurore::Insets notch = { 0, 48, 0, 48 };
    aurore::Application::setSafeAreaOverride(&notch);
    AURORE_CHECK(aurore::Application::getSafeArea() == notch);
    aurore::Application::setSafeAreaOverride(nullptr);
    AURORE_CHECK(aurore::Application::getSafeArea().empty());
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
