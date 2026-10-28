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

#include <aurore.hpp>
#include <aurore/test/harness.hpp>
#include <aurore/test/headless_platform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <thread>

#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

namespace aurore::test
{

// Image

Image Image::crop(int x, int y, int w, int h) const
{
    Image out;
    int x0 = std::clamp(x, 0, width), y0 = std::clamp(y, 0, height);
    int x1 = std::clamp(x + w, 0, width), y1 = std::clamp(y + h, 0, height);
    out.width  = x1 - x0;
    out.height = y1 - y0;
    out.rgba.resize((size_t)out.width * out.height * 4);
    for (int row = 0; row < out.height; row++)
        std::copy_n(at(x0, y0 + row), (size_t)out.width * 4, out.rgba.data() + (size_t)row * out.width * 4);
    return out;
}

bool Image::savePng(const std::string& path) const
{
    if (empty())
        return false;
    std::error_code error;
    std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty())
        std::filesystem::create_directories(parent, error);
    return stbi_write_png(path.c_str(), width, height, 4, rgba.data(), width * 4) != 0;
}

Image Image::loadPng(const std::string& path)
{
    Image image;
    int channels        = 0;
    unsigned char* data = stbi_load(path.c_str(), &image.width, &image.height, &channels, 4);
    if (!data)
    {
        image.width = image.height = 0;
        return image;
    }
    image.rgba.assign(data, data + (size_t)image.width * image.height * 4);
    stbi_image_free(data);
    return image;
}

ImageDiff compareImages(const Image& actual, const Image& expected, int tolerance)
{
    ImageDiff result;
    result.sameSize = actual.width == expected.width && actual.height == expected.height && !actual.empty();
    if (!result.sameSize)
    {
        result.differingPixels = (size_t)std::max(actual.width * actual.height, expected.width * expected.height);
        result.ratio           = 1;
        result.maxDelta        = 255;
        return result;
    }
    result.map    = expected;
    size_t pixels = (size_t)actual.width * actual.height;
    for (size_t i = 0; i < pixels; i++)
    {
        const unsigned char* a = actual.rgba.data() + i * 4;
        const unsigned char* e = expected.rgba.data() + i * 4;
        unsigned char* m       = result.map.rgba.data() + i * 4;
        int delta              = 0;
        for (int c = 0; c < 4; c++)
            delta = std::max(delta, std::abs((int)a[c] - (int)e[c]));
        result.maxDelta = std::max(result.maxDelta, delta);
        if (delta > tolerance)
        {
            result.differingPixels++;
            m[0] = 255;
            m[1] = 0;
            m[2] = 0;
        }
        else
        {
            // Faded, so the differences stand out
            for (int c = 0; c < 3; c++)
                m[c] = (unsigned char)(255 - (255 - m[c]) / 4);
        }
        m[3] = 255;
    }
    result.ratio = (double)result.differingPixels / (double)pixels;
    return result;
}

std::string viewText(View* view)
{
    if (!view)
        return "";
    if (auto* label = dynamic_cast<Label*>(view))
        return label->getFullText();
    if (auto* button = dynamic_cast<Button*>(view))
        return button->getText();
    if (auto* cell = dynamic_cast<DetailCell*>(view))
        return cell->title->getFullText();
    if (auto* box = dynamic_cast<Box*>(view))
    {
        for (View* child : box->getChildren())
        {
            if (child->getVisibility() != Visibility::VISIBLE)
                continue;
            std::string text = viewText(child);
            if (!text.empty())
                return text;
        }
    }
    return "";
}

static std::string fromEnvironment(const char* name, const std::string& fallback)
{
    const char* value = std::getenv(name);
    return value && *value ? std::string(value) : fallback;
}

static bool flagFromEnvironment(const char* name)
{
    const char* value = std::getenv(name);
    return value && *value && *value != '0';
}

// Harness

Harness::Harness(Options options)
    : opts(std::move(options))
{
    if (opts.referenceDir.empty())
        opts.referenceDir = fromEnvironment("AURORE_TEST_REFERENCES", "references");
    if (opts.outputDir.empty())
        opts.outputDir = fromEnvironment("AURORE_TEST_OUTPUT", "test-output");

    // Not 0: some code reads a time of 0 as "never"
    this->clock = 1000000;
    setSimulatedTimeUsec(this->clock);
    if (opts.wallTime >= 0)
    {
#ifdef _WIN32
        _putenv_s("TZ", "UTC");
        _tzset();
#else
        setenv("TZ", "UTC", 1);
        tzset();
#endif
        setSimulatedWallTime(opts.wallTime);
    }

    DesktopFontLoader::useSystemFonts = opts.systemFonts;

    Platform::setFactory([this]()
        {
        this->headless = new HeadlessPlatform(opts.width, opts.height, opts.locale, opts.theme);
        return this->headless; });

    if (!Application::init())
        fatal("test harness: Application::init() failed");
    Application::createWindow(opts.title);

    Application::getGlobalFocusChangeEvent()->subscribe([this](View* view)
        { this->focusChanges.push_back(viewText(view)); });
}

Harness::~Harness()
{
    if (this->headless)
    {
        this->headless->requestClose();
        Application::mainLoop(); // exits, and deletes the platform
    }
    Platform::setFactory(nullptr);
    setSimulatedWallTime(-1);
    setSimulatedTimeUsec(-1);
}

void Harness::push(Activity* activity, TransitionAnimation animation)
{
    Application::pushActivity(activity, animation);
    step();
}

Activity* Harness::top()
{
    auto stack = Application::getActivitiesStack();
    return stack.empty() ? nullptr : stack.back();
}

View* Harness::find(const std::string& id)
{
    Activity* activity = top();
    return activity ? activity->getView(id) : nullptr;
}

void Harness::step(int count)
{
    for (int i = 0; i < count; i++)
    {
        this->frames++;
        // From the frame count, so the clock does not drift with rounding
        this->clock = 1000000 + (Time)std::llround((double)this->frames * opts.frameMs * 1000.0);
        setSimulatedTimeUsec(this->clock);
        if (!Application::mainLoop())
            fatal("test harness: the main loop ended");
    }
}

void Harness::advance(double ms)
{
    step(std::max(1, (int)std::ceil(ms / opts.frameMs)));
}

bool Harness::settle(double maxMs)
{
    int quiet   = 0;
    int maximum = std::max(2, (int)std::ceil(maxMs / opts.frameMs));
    for (int i = 0; i < maximum; i++)
    {
        step();
        bool tasksIdle = Threading::isIdle();
        if (!tasksIdle)
        {
            // A task of the async thread runs in real time
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (tasksIdle && Ticking::runningTickings.empty())
        {
            if (++quiet >= 2)
                return true;
        }
        else
        {
            quiet = 0;
        }
    }
    return false;
}

Time Harness::now() const
{
    return this->clock;
}

void Harness::press(ControllerButton button)
{
    headless->getHeadlessInput()->setButton(button, true);
}

void Harness::release(ControllerButton button)
{
    headless->getHeadlessInput()->setButton(button, false);
}

void Harness::tap(ControllerButton button, int times)
{
    for (int i = 0; i < times; i++)
    {
        press(button);
        step();
        release(button);
        step();
        settle();
    }
}

void Harness::hold(ControllerButton button, double ms)
{
    press(button);
    advance(ms);
    release(button);
    step();
    settle();
}

void Harness::setAxis(ControllerAxis axis, float value)
{
    headless->getHeadlessInput()->setAxis(axis, value);
}

void Harness::pressKey(AuroreKeyboardScancode key)
{
    headless->getHeadlessInput()->setKey(key, true);
}

void Harness::releaseKey(AuroreKeyboardScancode key)
{
    headless->getHeadlessInput()->setKey(key, false);
}

void Harness::tapKey(AuroreKeyboardScancode key, int times)
{
    for (int i = 0; i < times; i++)
    {
        pressKey(key);
        step();
        releaseKey(key);
        step();
        settle();
    }
}

void Harness::touchDown(float x, float y, int finger)
{
    headless->getHeadlessInput()->setTouch(finger, true, Point(x, y));
}

void Harness::touchMove(float x, float y, int finger)
{
    headless->getHeadlessInput()->setTouch(finger, true, Point(x, y));
}

void Harness::touchUp(int finger)
{
    headless->getHeadlessInput()->setTouch(finger, false, Point());
}

void Harness::tapAt(float x, float y)
{
    touchDown(x, y);
    step();
    touchUp();
    step();
    settle();
}

bool Harness::tapView(View* view)
{
    if (!view)
        return false;
    Rect frame = view->getFrame();
    tapAt(frame.getMidX(), frame.getMidY());
    return true;
}

void Harness::swipe(float fromX, float fromY, float toX, float toY, double ms)
{
    int steps = std::max(2, (int)std::ceil(ms / opts.frameMs));
    touchDown(fromX, fromY);
    step();
    for (int i = 1; i <= steps; i++)
    {
        float t = (float)i / (float)steps;
        touchMove(fromX + (toX - fromX) * t, fromY + (toY - fromY) * t);
        step();
    }
    touchUp();
    step();
    settle();
}

void Harness::mouseMove(float x, float y)
{
    headless->getHeadlessInput()->setMouse(Point(x, y), false);
    step();
}

void Harness::mouseClick(float x, float y)
{
    headless->getHeadlessInput()->setMouse(Point(x, y), true);
    step();
    headless->getHeadlessInput()->setMouse(Point(x, y), false);
    step();
    settle();
}

void Harness::mouseQuickClick(float x, float y)
{
    headless->getHeadlessInput()->setMouse(Point(x, y), true);
    headless->getHeadlessInput()->setMouse(Point(x, y), false);
    step();
    step();
    settle();
}

void Harness::mouseScroll(float dx, float dy)
{
    headless->getHeadlessInput()->scroll(Point(dx, dy));
    step();
    settle();
}

void Harness::setKeyboardAnswer(const std::string& text)
{
    headless->getHeadlessIme()->nextText = text;
}

View* Harness::focus()
{
    return Application::getCurrentFocus();
}

std::string Harness::focusText()
{
    return viewText(focus());
}

bool Harness::focusByText(const std::string& text, ControllerButton button, int maxSteps)
{
    for (int i = 0; i < maxSteps; i++)
    {
        if (focusText() == text)
            return true;
        tap(button);
    }
    return focusText() == text;
}

std::vector<std::string> Harness::silentFocusables()
{
    std::vector<std::string> silent;
    walkShown([&silent](View* view)
        {
        if (view->isFocusable() && view->getDefaultFocus() == view && view->getAccessibilityLabel().empty()
            && view->getAccessibilityValue().empty())
            silent.push_back(view->describe()); });
    return silent;
}

void Harness::walkShown(const std::function<void(View*)>& visit)
{
    std::function<void(View*)> walk = [&](View* view)
    {
        if (!view || view->getVisibility() != Visibility::VISIBLE)
            return;
        visit(view);
        if (auto* box = dynamic_cast<Box*>(view))
            for (View* child : box->getChildren())
                walk(child);
    };
    auto stack = Application::getActivitiesStack();
    // From the bottom activity drawn up, as frame() draws them
    size_t first = stack.size();
    while (first > 0)
    {
        first--;
        if (!stack[first]->isTranslucent())
            break;
    }
    for (size_t i = first; i < stack.size(); i++)
        walk(stack[i]->getContentView());
}

std::vector<std::string> Harness::cutTexts()
{
    std::vector<std::string> cut;
    walkShown([&](View* view)
        {
        auto* label = dynamic_cast<Label*>(view);
        if (label && label->getWidth() > 0 && label->isTruncated())
            cut.push_back(label->getFullText()); });
    return cut;
}

std::vector<std::string> Harness::texts()
{
    std::vector<std::string> all;
    walkShown([&](View* view)
        {
        if (auto* label = dynamic_cast<Label*>(view))
            all.push_back(label->getFullText()); });
    return all;
}

bool Harness::hasText(const std::string& text)
{
    return findText(text) != nullptr;
}

View* Harness::findText(const std::string& text)
{
    View* found = nullptr;
    walkShown([&](View* view)
        {
        auto* label = dynamic_cast<Label*>(view);
        if (!found && label && label->getFullText() == text)
            found = label; });
    return found;
}

Image Harness::screenshot()
{
    if (Application::getDrawnFrameCount() == 0)
        step();
    Image image;
    if (!headless->getVideoContext()->readPixels(&image.rgba, &image.width, &image.height))
        image = Image();
    return image;
}

bool Harness::matchesReference(const std::string& name, int tolerance, double maxRatio)
{
    return matchesReference(name, Rect(0, 0, 0, 0), tolerance, maxRatio);
}

bool Harness::matchesReference(const std::string& name, const Rect& area, int tolerance, double maxRatio)
{
    Image actual = screenshot();
    if (area.getWidth() > 0 && area.getHeight() > 0)
    {
        float scale = Application::windowScale;
        actual      = actual.crop((int)std::floor(area.getMinX() * scale), (int)std::floor(area.getMinY() * scale),
                 (int)std::ceil(area.getWidth() * scale), (int)std::ceil(area.getHeight() * scale));
    }

    std::string referencePath = opts.referenceDir + "/" + name + ".png";
    std::string outputPath    = opts.outputDir + "/" + name + ".png";
    std::string diffPath      = opts.outputDir + "/" + name + ".diff.png";
    actual.savePng(outputPath);

    Image expected = Image::loadPng(referencePath);
    if (expected.empty() && !flagFromEnvironment("AURORE_TEST_UPDATE") && std::getenv("CI"))
    {
        // In CI a missing reference is a test that was never checked: a typo in its name, or a file not committed
        Logger::error("test harness: no reference {} (run with AURORE_TEST_UPDATE=1 to write it, then commit it)",
            referencePath);
        this->diff = ImageDiff();
        return false;
    }
    if (expected.empty() || flagFromEnvironment("AURORE_TEST_UPDATE"))
    {
        Logger::info("test harness: reference {} written", referencePath);
        actual.savePng(referencePath);
        this->diff          = ImageDiff();
        this->diff.sameSize = true;
        return true;
    }

    this->diff   = compareImages(actual, expected, tolerance);
    bool matches = this->diff.sameSize && this->diff.ratio <= maxRatio;
    if (!matches)
    {
        if (this->diff.sameSize)
            this->diff.map.savePng(diffPath);
        Logger::error("test harness: {} differs from {}: {} pixels ({:.3f}%), largest difference {} (see {})",
            name, referencePath, this->diff.differingPixels, this->diff.ratio * 100.0, this->diff.maxDelta,
            this->diff.sameSize ? diffPath : std::string("the sizes"));
    }
    else
    {
        std::error_code error;
        std::filesystem::remove(diffPath, error);
    }
    return matches;
}

void Harness::resize(int width, int height)
{
    headless->setWindowSize(width, height);
    step();
    settle();
}

// Runner

namespace
{
    struct TestCase
    {
        const char* name;
        TestFunction function;
    };

    std::vector<TestCase>& registry()
    {
        static std::vector<TestCase> tests;
        return tests;
    }

    int failures = 0;
}

bool registerTest(const char* name, TestFunction function)
{
    registry().push_back({ name, function });
    return true;
}

void reportFailure(const char* file, int line, const std::string& what)
{
    failures++;
    std::fprintf(stderr, "%s:%d: FAILED: %s\n", file, line, what.c_str());
    std::fflush(stderr);
}

int runTests(int argc, char** argv)
{
    auto& tests = registry();
    std::sort(tests.begin(), tests.end(), [](const TestCase& a, const TestCase& b)
        { return std::string(a.name) < std::string(b.name); });

    if (argc > 1 && std::string(argv[1]) == "--list")
    {
        // Marked: the program may log before (aurore_add_tests reads them)
        for (auto& test : tests)
            std::printf("aurore-test: %s\n", test.name);
        return 0;
    }

    if (argc > 1)
    {
        for (auto& test : tests)
        {
            if (argv[1] == std::string(test.name))
            {
                test.function();
                std::printf("%s: %s\n", test.name, failures ? "FAILED" : "passed");
                return failures ? 1 : 0;
            }
        }
        std::fprintf(stderr, "no test named %s (--list lists them)\n", argv[1]);
        return 2;
    }

    // Each case in a process of its own: borealis' Application is global
    int failed = 0;
    for (auto& test : tests)
    {
        std::string command = std::string("\"") + argv[0] + "\" " + test.name;
        int status          = std::system(command.c_str());
        if (status != 0)
            failed++;
    }
    std::printf("%zu tests, %d failed\n", tests.size(), failed);
    return failed ? 1 : 0;
}

} // namespace aurore::test
