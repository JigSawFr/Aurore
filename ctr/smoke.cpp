// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// aurore_3ds_smoke: Aurore on the New 3DS, drawn by citro3d. A test card on
// the bottom screen: a layout made from XML (boxes, labels) with the small
// screen preset, and a view that draws every kind of paint and fill nanovg's
// citro3d renderer handles (colour, gradients, image, nonzero fill, hole,
// strokes, scissor, text). Three buttons and a line for the input: the
// D-pad, the Circle Pad or the C-stick move the focus, A or a touch clicks,
// the line shows the buttons held, the sticks and the touch. The log goes to
// the top screen. START quits (docs/platforms.md).

#include <3ds.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <aurore.hpp>
#include <aurore/version.hpp>

namespace
{

const char* const LAYOUT = R"xml(
<aurore:Box axis="column" width="100%" height="100%" padding="12dp" backgroundColor="#101828">
    <aurore:Label id="title" fontSize="22" textColor="#F5F7FF" marginBottom="4dp" />
    <aurore:Label id="details" fontSize="15" textColor="#9AA8C7" marginBottom="8dp" />
    <aurore:Box axis="row" marginBottom="6dp">
        <aurore:Button id="one" text="One" grow="1" marginRight="6dp" style="primary" />
        <aurore:Button id="two" text="Two" grow="1" marginRight="6dp" />
        <aurore:Button id="three" text="Three" grow="1" />
    </aurore:Box>
    <aurore:Label id="input" fontSize="13" textColor="#9AA8C7" />
</aurore:Box>
)xml";

// Six cells: what the renderer draws, one kind each
class TestCard : public aurore::Box
{
  public:
    TestCard() { this->setGrow(1.0f); }

    ~TestCard() override
    {
        if (this->image)
            nvgDeleteImage(aurore::Application::getNVGContext(), this->image);
    }

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style,
        aurore::FrameContext* ctx) override
    {
        if (!this->image)
            this->image = nvgCreateImage(vg, AURORE_ASSET("img/demo_icon.jpg"), 0);

        float cw = width / 3;
        float ch = height / 2;
        float r  = std::min(cw, ch) * 0.34f;
        auto cx  = [&](int col)
        { return x + cw * (col + 0.5f); };
        auto cy = [&](int row)
        { return y + ch * (row + 0.42f); };

        // A colour, rounded, with a stroke (convex fill, strip)
        nvgBeginPath(vg);
        nvgRoundedRect(vg, cx(0) - r, cy(0) - r, 2 * r, 2 * r, r * 0.3f);
        nvgFillColor(vg, nvgRGB(0x3B, 0x82, 0xF6));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGB(0xF5, 0xF7, 0xFF));
        nvgStrokeWidth(vg, 3);
        nvgStroke(vg);

        // A linear gradient
        nvgBeginPath(vg);
        nvgRect(vg, cx(1) - r, cy(0) - r, 2 * r, 2 * r);
        nvgFillPaint(vg, nvgLinearGradient(vg, cx(1) - r, cy(0) - r, cx(1) + r, cy(0) + r, nvgRGB(0x22, 0xD3, 0xEE), nvgRGB(0xA8, 0x55, 0xF7)));
        nvgFill(vg);

        // A radial gradient, half clipped by the scissor
        nvgSave(vg);
        nvgScissor(vg, cx(2) - r, cy(0) - r, 2 * r, r * 1.4f);
        nvgBeginPath(vg);
        nvgCircle(vg, cx(2), cy(0), r);
        nvgFillPaint(vg, nvgRadialGradient(vg, cx(2), cy(0), r * 0.2f, r, nvgRGB(0xFA, 0xCC, 0x15), nvgRGBA(0xF9, 0x73, 0x16, 0x40)));
        nvgFill(vg);
        nvgRestore(vg);

        // A box gradient: a shadow under a card
        nvgBeginPath(vg);
        nvgRect(vg, cx(0) - r - 10, cy(1) - r - 10, 2 * r + 20, 2 * r + 24);
        nvgFillPaint(vg, nvgBoxGradient(vg, cx(0) - r, cy(1) - r + 4, 2 * r, 2 * r, 8, 10, nvgRGBA(0, 0, 0, 200), nvgRGBA(0, 0, 0, 0)));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, cx(0) - r, cy(1) - r, 2 * r, 2 * r, 8);
        nvgFillColor(vg, nvgRGB(0x1F, 0x29, 0x37));
        nvgFill(vg);

        // A star (nonzero, through the stencil) with a hole
        nvgBeginPath(vg);
        for (int i = 0; i < 10; i++)
        {
            float a  = NVG_PI * i / 5 - NVG_PI / 2;
            float rr = i % 2 ? r * 0.45f : r;
            if (i == 0)
                nvgMoveTo(vg, cx(1) + std::cos(a) * rr, cy(1) + std::sin(a) * rr);
            else
                nvgLineTo(vg, cx(1) + std::cos(a) * rr, cy(1) + std::sin(a) * rr);
        }
        nvgClosePath(vg);
        nvgCircle(vg, cx(1), cy(1), r * 0.2f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillColor(vg, nvgRGB(0x34, 0xD3, 0x99));
        nvgFill(vg);

        // An image, rounded
        if (this->image)
        {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, cx(2) - r, cy(1) - r, 2 * r, 2 * r, r * 0.3f);
            nvgFillPaint(vg, nvgImagePattern(vg, cx(2) - r, cy(1) - r, 2 * r, 2 * r, 0, this->image, 1.0f));
            nvgFill(vg);
        }

        // The cells' names (text: the font atlas)
        const char* names[6] = { "colour", "linear", "radial", "box", "nonzero", "image" };
        nvgFontFaceId(vg, aurore::Application::getFont(aurore::FONT_REGULAR));
        nvgFontSize(vg, 14);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGB(0xC7, 0xD2, 0xFE));
        for (int i = 0; i < 6; i++)
            nvgText(vg, cx(i % 3), cy(i / 3) + r + 6, names[i], nullptr);
    }

  private:
    int image = 0;
};

// What Aurore reads this frame, in a line
std::string describeInput()
{
    static const char* const NAMES[aurore::_BUTTON_MAX] = { "ZL", "L", nullptr, "Up", "Right", "Down", "Left",
        "SELECT", nullptr, "START", nullptr, "Y", "B", "A", "X", "R", "ZR", nullptr, nullptr, nullptr, nullptr };
    const aurore::ControllerState& state                = aurore::Application::getControllerState();
    std::string line;
    for (int i = 0; i < aurore::_BUTTON_MAX; i++)
        if (NAMES[i] && state.buttons[i])
            line += std::string(NAMES[i]) + " ";
    char sticks[64];
    std::snprintf(sticks, sizeof(sticks), "pad %+.2f %+.2f  C %+.2f %+.2f", state.axes[aurore::LEFT_X],
        state.axes[aurore::LEFT_Y], state.axes[aurore::RIGHT_X], state.axes[aurore::RIGHT_Y]);
    line += sticks;
    if (hidKeysHeld() & KEY_TOUCH)
    {
        touchPosition touch;
        hidTouchRead(&touch);
        line += "  touch " + std::to_string(touch.px) + "," + std::to_string(touch.py);
    }
    return line;
}

// A button clicked (A or a touch), in the log
bool logClick(aurore::View* view)
{
    std::printf("Clicked %s\n", view->getId().c_str());
    return true;
}

} // namespace

int main(int argc, char** argv)
{
    if (!aurore::Application::init())
        return 1;
    aurore::Application::createWindow("Aurore");
    aurore::Application::setGlobalQuit(false);
    std::printf("Aurore %s on the %s\n", AURORE_VERSION, aurore::Application::getPlatform()->getName().c_str());
    std::printf("Design size %ux%u, density %.2f\n", aurore::Application::ORIGINAL_WINDOW_WIDTH,
        aurore::Application::ORIGINAL_WINDOW_HEIGHT, aurore::getDensity());
    std::printf("START quits.\n\n");

    auto* root = dynamic_cast<aurore::Box*>(aurore::View::createFromXMLString(LAYOUT));
    if (!root)
        return 1;
    if (auto* title = dynamic_cast<aurore::Label*>(root->getView("title")))
        title->setText("Aurore on citro3d");
    if (auto* details = dynamic_cast<aurore::Label*>(root->getView("details")))
        details->setText(aurore::Application::getPlatform()->getName() + ", Aurore " + AURORE_VERSION);
    auto* input = dynamic_cast<aurore::Label*>(root->getView("input"));
    for (const char* id : { "one", "two", "three" })
        root->getView(id)->registerClickAction(logClick);
    root->addView(new TestCard());
    aurore::Application::pushActivity(new aurore::Activity(root));

    std::string shown;
    while (aurore::Application::mainLoop())
    {
        // The platform scans the buttons each frame
        if (hidKeysDown() & KEY_START)
            break;
        std::string line = describeInput();
        if (input && line != shown)
        {
            input->setText(line);
            shown = line;
        }
    }
    return 0;
}
