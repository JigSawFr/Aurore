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

// openBrowser(): which links it accepts, and that a link reaches the opener
// as one argument, never through a shell

#include <borealis/core/platform.hpp>
#include <borealis/test/harness.hpp>

#if defined(__linux__) && !defined(ANDROID)
#include <sys/wait.h>
#include <unistd.h>

#include <borealis/platforms/desktop/desktop_platform.hpp>
#include <borealis/platforms/desktop/steam_deck.hpp>
#endif

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using brls::Platform;

BRLS_TEST(browser_urls_are_http_or_https)
{
    BRLS_CHECK(Platform::isBrowserUrl("https://github.com/JigSawFr/Aurore"));
    BRLS_CHECK(Platform::isBrowserUrl("http://example.org/a?b=c&d=e#f"));
    BRLS_CHECK(Platform::isBrowserUrl("HTTPS://EXAMPLE.ORG"));
    // Characters a shell would read are only text in a link
    BRLS_CHECK(Platform::isBrowserUrl("https://example.org/$(id)`id`\"'"));

    BRLS_CHECK(!Platform::isBrowserUrl(""));
    BRLS_CHECK(!Platform::isBrowserUrl("https://"));
    BRLS_CHECK(!Platform::isBrowserUrl("example.org"));
    BRLS_CHECK(!Platform::isBrowserUrl("://example.org"));
    BRLS_CHECK(!Platform::isBrowserUrl("file:///etc/passwd"));
    BRLS_CHECK(!Platform::isBrowserUrl("javascript://alert(1)"));
    BRLS_CHECK(!Platform::isBrowserUrl("steam://run/440"));
    BRLS_CHECK(!Platform::isBrowserUrl("httpx://example.org"));
    BRLS_CHECK(!Platform::isBrowserUrl("-https://example.org"));
    BRLS_CHECK(!Platform::isBrowserUrl("https://example.org/\nsteam://run/440"));
    BRLS_CHECK(!Platform::isBrowserUrl("https://example.org/\r"));
    BRLS_CHECK(!Platform::isBrowserUrl(std::string("https://example.org/\0x", 22)));
    BRLS_CHECK(!Platform::isBrowserUrl("https://example.org/\x1b[2J"));
    BRLS_CHECK(!Platform::isBrowserUrl("https://example.org/\x7f"));
}

BRLS_TEST(browser_url_schemes_can_be_widened)
{
    auto saved = Platform::BROWSER_URL_SCHEMES;
    BRLS_CHECK(!Platform::isBrowserUrl("mailto://someone@example.org"));
    Platform::BROWSER_URL_SCHEMES.push_back("mailto");
    BRLS_CHECK(Platform::isBrowserUrl("mailto://someone@example.org"));
    Platform::BROWSER_URL_SCHEMES = saved;
}

#if defined(__linux__) && !defined(ANDROID)

namespace
{

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

BRLS_TEST(open_url_passes_the_link_as_one_argument)
{
    std::vector<std::string> cmd = brls::DesktopPlatform::browserCommand("https://x/$(id)");
    BRLS_CHECK(cmd.size() == 2);
    BRLS_CHECK_EQ(cmd[0], std::string("xdg-open"));
    BRLS_CHECK_EQ(cmd[1], std::string("https://x/$(id)"));
    BRLS_CHECK(brls::DesktopPlatform::browserCommand("file:///etc/passwd").empty());
    BRLS_CHECK(brls::DesktopPlatform::browserCommand("https://x/\nfoo").empty());
    BRLS_CHECK_EQ(brls::DesktopPlatform::spawnProcess({}), -1);

    // A stand-in xdg-open, first in PATH, that writes down its arguments
    std::filesystem::path dir = std::filesystem::temp_directory_path() / ("aurore-open-url-" + std::to_string(getpid()));
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    std::filesystem::path opener = dir / "xdg-open";
    {
        std::ofstream out(opener);
        out << "#!/bin/sh\nprintf '%s|' \"$#\" \"$@\" > \"" << (dir / "args").string() << "\"\n";
    }
    std::filesystem::permissions(opener, std::filesystem::perms::owner_all);

    const char* oldPath   = getenv("PATH");
    std::string savedPath = oldPath ? oldPath : "";
    setenv("PATH", (dir.string() + ":" + savedPath).c_str(), 1);

    std::string pwned = (dir / "pwned").string();
    std::string url   = "https://example.org/$(touch " + pwned + ")`touch " + pwned + "`\"; touch " + pwned + "; \"'";
    int pid           = brls::DesktopPlatform::spawnProcess(brls::DesktopPlatform::browserCommand(url));
    BRLS_CHECK(pid > 0);
    int status = 0;
    if (pid > 0)
        waitpid(pid, &status, 0);
    setenv("PATH", savedPath.c_str(), 1);

    BRLS_CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    BRLS_CHECK_EQ(readFile(dir / "args"), "1|" + url + "|");
    BRLS_CHECK(!std::filesystem::exists(pwned));
    std::filesystem::remove_all(dir);
}

BRLS_TEST(steam_open_url_is_one_line)
{
    BRLS_CHECK_EQ(brls::steamOpenUrlCommand("https://example.org/a"), std::string("steam://openurl/https://example.org/a\n"));
    BRLS_CHECK(brls::steamOpenUrlCommand("https://example.org/\nsteam://run/440").empty());
    BRLS_CHECK(brls::steamOpenUrlCommand("steam://run/440").empty());
}

#endif

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}
