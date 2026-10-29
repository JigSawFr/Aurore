// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// openBrowser(): which links it accepts, and that a link reaches the opener
// as one argument, never through a shell

#include <aurore/core/platform.hpp>
#include <aurore/test/harness.hpp>

#if defined(__linux__) && !defined(ANDROID)
#include <sys/wait.h>
#include <unistd.h>

#include <aurore/platforms/desktop/desktop_platform.hpp>
#include <aurore/platforms/desktop/steam_deck.hpp>
#endif

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using aurore::Platform;

AURORE_TEST(browser_urls_are_http_or_https)
{
    AURORE_CHECK(Platform::isBrowserUrl("https://github.com/JigSawFr/Aurore"));
    AURORE_CHECK(Platform::isBrowserUrl("http://example.org/a?b=c&d=e#f"));
    AURORE_CHECK(Platform::isBrowserUrl("HTTPS://EXAMPLE.ORG"));
    // Characters a shell would read are only text in a link
    AURORE_CHECK(Platform::isBrowserUrl("https://example.org/$(id)`id`\"'"));

    AURORE_CHECK(!Platform::isBrowserUrl(""));
    AURORE_CHECK(!Platform::isBrowserUrl("https://"));
    AURORE_CHECK(!Platform::isBrowserUrl("example.org"));
    AURORE_CHECK(!Platform::isBrowserUrl("://example.org"));
    AURORE_CHECK(!Platform::isBrowserUrl("file:///etc/passwd"));
    AURORE_CHECK(!Platform::isBrowserUrl("javascript://alert(1)"));
    AURORE_CHECK(!Platform::isBrowserUrl("steam://run/440"));
    AURORE_CHECK(!Platform::isBrowserUrl("httpx://example.org"));
    AURORE_CHECK(!Platform::isBrowserUrl("-https://example.org"));
    AURORE_CHECK(!Platform::isBrowserUrl("https://example.org/\nsteam://run/440"));
    AURORE_CHECK(!Platform::isBrowserUrl("https://example.org/\r"));
    AURORE_CHECK(!Platform::isBrowserUrl(std::string("https://example.org/\0x", 22)));
    AURORE_CHECK(!Platform::isBrowserUrl("https://example.org/\x1b[2J"));
    AURORE_CHECK(!Platform::isBrowserUrl("https://example.org/\x7f"));
}

AURORE_TEST(browser_url_schemes_can_be_widened)
{
    auto saved = Platform::BROWSER_URL_SCHEMES;
    AURORE_CHECK(!Platform::isBrowserUrl("mailto://someone@example.org"));
    Platform::BROWSER_URL_SCHEMES.push_back("mailto");
    AURORE_CHECK(Platform::isBrowserUrl("mailto://someone@example.org"));
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

AURORE_TEST(open_url_passes_the_link_as_one_argument)
{
    std::vector<std::string> cmd = aurore::DesktopPlatform::browserCommand("https://x/$(id)");
    AURORE_CHECK(cmd.size() == 2);
    AURORE_CHECK_EQ(cmd[0], std::string("xdg-open"));
    AURORE_CHECK_EQ(cmd[1], std::string("https://x/$(id)"));
    AURORE_CHECK(aurore::DesktopPlatform::browserCommand("file:///etc/passwd").empty());
    AURORE_CHECK(aurore::DesktopPlatform::browserCommand("https://x/\nfoo").empty());
    AURORE_CHECK_EQ(aurore::DesktopPlatform::spawnProcess({}), -1);

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
    int pid           = aurore::DesktopPlatform::spawnProcess(aurore::DesktopPlatform::browserCommand(url));
    AURORE_CHECK(pid > 0);
    int status = 0;
    if (pid > 0)
        waitpid(pid, &status, 0);
    setenv("PATH", savedPath.c_str(), 1);

    AURORE_CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    AURORE_CHECK_EQ(readFile(dir / "args"), "1|" + url + "|");
    AURORE_CHECK(!std::filesystem::exists(pwned));
    std::filesystem::remove_all(dir);
}

AURORE_TEST(steam_open_url_is_one_line)
{
    AURORE_CHECK_EQ(aurore::steamOpenUrlCommand("https://example.org/a"), std::string("steam://openurl/https://example.org/a\n"));
    AURORE_CHECK(aurore::steamOpenUrlCommand("https://example.org/\nsteam://run/440").empty());
    AURORE_CHECK(aurore::steamOpenUrlCommand("steam://run/440").empty());
}

#endif

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
