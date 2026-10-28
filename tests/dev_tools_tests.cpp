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

#include <borealis/cadran/ui/prefs.hpp>
#include <borealis/core/application.hpp>
#include <borealis/core/hot_reload.hpp>
#include <borealis/core/inspector.hpp>
#include <borealis/core/profiler.hpp>
#include <borealis/test/harness.hpp>

#include <cstdlib>

using brls::test::Harness;

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
    setenv("BRLS_SAFE_AREA", "10,20", 1);

    Harness harness;
    harness.step(2);
    BRLS_CHECK_EQ(brls::Inspector::isEnabled(), BUILT_IN);
    BRLS_CHECK_EQ(brls::HotReload::isEnabled(), BUILT_IN);
    BRLS_CHECK_EQ(brls::Application::isRedrawDebug(), BUILT_IN);
    BRLS_CHECK_EQ(brls::Profiler::isEnabled(), BUILT_IN);
    BRLS_CHECK_EQ(ui::env_flag("STILL_FOCUS"), BUILT_IN);
    BRLS_CHECK_EQ(brls::Application::getSafeArea() == (brls::Insets { 10, 20, 10, 20 }), BUILT_IN);
}

BRLS_TEST(the_debugging_aids_functions_work_in_every_build)
{
    Harness harness;
    brls::Application::setInspectorEnabled(true);
    BRLS_CHECK(brls::Inspector::isEnabled());
    brls::Application::setInspectorEnabled(false);
    brls::Profiler::setEnabled(true);
    BRLS_CHECK(brls::Profiler::isEnabled());
    brls::Profiler::setEnabled(false);
    brls::Application::setRedrawDebug(true);
    BRLS_CHECK(brls::Application::isRedrawDebug());
    brls::Application::setRedrawDebug(false);
    const brls::Insets notch = { 0, 48, 0, 48 };
    brls::Application::setSafeAreaOverride(&notch);
    BRLS_CHECK(brls::Application::getSafeArea() == notch);
    brls::Application::setSafeAreaOverride(nullptr);
    BRLS_CHECK(brls::Application::getSafeArea().empty());
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}
