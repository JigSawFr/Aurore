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
// BRLS_DEV_TOOLS (not a release one, by default); their functions work in
// every build

#include <aurore/cadran/ui/prefs.hpp>
#include <aurore/core/application.hpp>
#include <aurore/core/hot_reload.hpp>
#include <aurore/core/inspector.hpp>
#include <aurore/core/profiler.hpp>
#include <aurore/test/harness.hpp>

#include <cstdlib>

using aurore::test::Harness;

#ifdef BRLS_DEV_TOOLS
static const bool BUILT_IN = true;
#else
static const bool BUILT_IN = false;
#endif

BRLS_TEST(environment_turns_the_debugging_aids_on_only_when_built_in)
{
    setenv("BRLS_INSPECTOR", "1", 1);
    setenv("BRLS_HOT_RELOAD", "1", 1);
    setenv("BRLS_REDRAW_DEBUG", "1", 1);
    setenv("BRLS_PROFILE", "1", 1);
    setenv("BRLS_STILL_FOCUS", "1", 1);

    Harness harness;
    harness.step(2);
    BRLS_CHECK_EQ(aurore::Inspector::isEnabled(), BUILT_IN);
    BRLS_CHECK_EQ(aurore::HotReload::isEnabled(), BUILT_IN);
    BRLS_CHECK_EQ(aurore::Application::isRedrawDebug(), BUILT_IN);
    BRLS_CHECK_EQ(aurore::Profiler::isEnabled(), BUILT_IN);
    BRLS_CHECK_EQ(ui::env_flag("STILL_FOCUS"), BUILT_IN);
}

BRLS_TEST(the_debugging_aids_functions_work_in_every_build)
{
    Harness harness;
    aurore::Application::setInspectorEnabled(true);
    BRLS_CHECK(aurore::Inspector::isEnabled());
    aurore::Application::setInspectorEnabled(false);
    aurore::Profiler::setEnabled(true);
    BRLS_CHECK(aurore::Profiler::isEnabled());
    aurore::Profiler::setEnabled(false);
    aurore::Application::setRedrawDebug(true);
    BRLS_CHECK(aurore::Application::isRedrawDebug());
    aurore::Application::setRedrawDebug(false);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
