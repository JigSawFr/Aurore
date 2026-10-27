// Inspector — an overlay over the screen showing the view tree, the focus
// chain, the yoga box and the theme values of a view, and the frame's timings.
// Copyright (C) 2026 JigSawFr.  GPLv3-or-later (see LICENSE.GPL).
#include <yoga/Yoga.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <typeinfo>
#include <borealis/core/application.hpp>
#include <borealis/core/box.hpp>
#include <borealis/core/frame_context.hpp>
#include <borealis/core/input.hpp>
#include <borealis/core/inspector.hpp>
#include <borealis/core/profiler.hpp>
#include <borealis/views/label.hpp>

#ifndef _MSC_VER
#include <cxxabi.h>
#endif

namespace brls
{

// Named: in a unity build, the other files' helpers do not meet these
namespace inspector_detail
{
    struct Row
    {
        View* view = nullptr; // nullptr: the heading of an activity
        int depth  = 0;
        std::string heading;
    };

    // What the panel shows of a frame the Profiler measured
    struct Sample
    {
        float frame   = 0; // ms since the frame before
        float views   = 0; // ms drawing the views, highlight and notifications
        float layout  = 0; // ms in yoga
        float flush   = 0; // ms in nvgEndFrame
        float present = 0; // ms presenting it
        int layouts   = 0;
        int draws     = 0;
        int triangles = 0;
    };

    constexpr int HISTORY = 120;
    constexpr int AVERAGE = 30;

    constexpr float PANEL_WIDTH = 460;
    constexpr float PAD         = 10;
    constexpr float ROW         = 16;
    constexpr float LINE        = 15;
    constexpr float TREE_TOP    = 124;
    constexpr int TREE_ROWS     = 17;

    const NVGcolor PANEL     = nvgRGBA(16, 18, 24, 235);
    const NVGcolor TEXT      = nvgRGB(226, 229, 236);
    const NVGcolor DIM       = nvgRGB(138, 145, 160);
    const NVGcolor ACCENT    = nvgRGB(255, 176, 64);
    const NVGcolor FOCUS     = nvgRGB(96, 205, 255);
    const NVGcolor SELECTION = nvgRGBA(255, 176, 64, 50);
    const NVGcolor WARN      = nvgRGB(255, 120, 110);

    // A view that takes the focus with nothing to tell it from another (its
    // role at most)
    bool silent(View* view)
    {
        return view->isFocusable() && view->getDefaultFocus() == view && view->getAccessibilityLabel().empty()
            && view->getAccessibilityValue().empty();
    }

    std::vector<Row> rows;
    View* selected               = nullptr;
    int scroll                   = 0;
    bool panelLeft               = false;
    bool keyHeld                 = false;
    bool comboHeld               = false;
    bool prev[_BUTTON_MAX]       = {};
    Time nextRepeat[_BUTTON_MAX] = {};

    Sample history[HISTORY];
    int historyEnd   = 0;
    int historyCount = 0;

    std::vector<std::pair<std::string, NVGcolor>> colors;
    std::vector<std::pair<std::string, float>> metrics;

    std::string typeName(const std::type_info& type)
    {
        const char* name   = type.name();
        std::string result = name;
#ifndef _MSC_VER
        int status      = 0;
        char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
        if (status == 0 && demangled)
            result = demangled;
        std::free(demangled);
#endif
        for (const char* prefix : { "brls::", "(anonymous namespace)::" })
        {
            size_t at;
            while ((at = result.find(prefix)) != std::string::npos)
                result.erase(at, std::strlen(prefix));
        }
        return result;
    }

    std::string shortName(View* view)
    {
        std::string name = typeName(typeid(*view));
        if (!view->getId().empty())
            name += " #" + view->getId();
        return name;
    }

    void addChildren(View* view, int depth)
    {
        rows.push_back({ view, depth, "" });
        if (Box* box = dynamic_cast<Box*>(view))
            for (View* child : box->getChildren())
                addChildren(child, depth + 1);
    }

    // Built again before each use: views come and go between frames
    void buildRows()
    {
        rows.clear();
        std::vector<Activity*> stack = Application::getActivitiesStack();
        for (auto it = stack.rbegin(); it != stack.rend(); ++it)
        {
            rows.push_back({ nullptr, 0, typeName(typeid(**it)) });
            if (View* content = (*it)->getContentView())
                addChildren(content, 1);
        }
    }

    int rowOf(View* view)
    {
        for (size_t i = 0; i < rows.size(); i++)
            if (rows[i].view == view)
                return (int)i;
        return -1;
    }

    void move(int delta)
    {
        if (rows.empty())
            return;
        int index = rowOf(selected);
        if (index < 0)
            index = 0;
        int step = delta < 0 ? -1 : 1;
        int left = std::abs(delta);
        int at   = index;
        while (left > 0)
        {
            int next = at + step;
            while (next >= 0 && next < (int)rows.size() && !rows[next].view)
                next += step;
            if (next < 0 || next >= (int)rows.size())
                break;
            at = next;
            left--;
        }
        if (rows[at].view)
            selected = rows[at].view;
    }

    bool isInFocusChain(View* view)
    {
        for (View* v = Application::getCurrentFocus(); v; v = v->hasParent() ? v->getParent() : nullptr)
            if (v == view)
                return true;
        return false;
    }

    std::string format(const char* text, float a)
    {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), text, a);
        return buffer;
    }

    std::string number(float value)
    {
        if (std::isnan(value))
            return "-";
        if (std::fabs(value - std::round(value)) < 0.05f)
            return format("%.0f", value);
        return format("%.1f", value);
    }

    std::string yogaValue(YGValue value)
    {
        switch (value.unit)
        {
            case YGUnitAuto:
                return "auto";
            case YGUnitPoint:
                return number(value.value);
            case YGUnitPercent:
                return number(value.value) + "%";
            default:
                return "-";
        }
    }

    std::string edges(YGNodeConstRef node, float (*get)(YGNodeConstRef, YGEdge))
    {
        float top = get(node, YGEdgeTop), right = get(node, YGEdgeRight);
        float bottom = get(node, YGEdgeBottom), left = get(node, YGEdgeLeft);
        if (top == right && right == bottom && bottom == left)
            return number(top);
        return number(top) + " " + number(right) + " " + number(bottom) + " " + number(left);
    }

    std::string hex(NVGcolor color)
    {
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x%02x", (int)std::lround(color.r * 255),
            (int)std::lround(color.g * 255), (int)std::lround(color.b * 255), (int)std::lround(color.a * 255));
        return buffer;
    }

    float text(NVGcontext* vg, float x, float y, NVGcolor color, const std::string& string)
    {
        nvgFillColor(vg, color);
        return nvgText(vg, x, y, string.c_str(), nullptr);
    }

    void swatch(NVGcontext* vg, float x, float y, NVGcolor color)
    {
        nvgBeginPath(vg);
        nvgRect(vg, x, y + 2, 10, 10);
        nvgFillColor(vg, color);
        nvgFill(vg);
        nvgStrokeColor(vg, DIM);
        nvgStrokeWidth(vg, 1);
        nvgStroke(vg);
    }

    // The theme key a colour property follows (View::setThemeColor), if any
    float themeKey(NVGcontext* vg, float x, float y, View* view, const char* property)
    {
        std::string key = view->getThemeColor(property);
        return key.empty() ? x : text(vg, x + 6, y, DIM, key);
    }

    // The area between two rectangles, filled
    void ring(NVGcontext* vg, float x, float y, float w, float h, float top, float right, float bottom, float left,
        NVGcolor color)
    {
        if (top == 0 && right == 0 && bottom == 0 && left == 0)
            return;
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgRect(vg, x + left, y + top, std::max(0.0f, w - left - right), std::max(0.0f, h - top - bottom));
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillColor(vg, color);
        nvgFill(vg);
    }

    void drawBox(NVGcontext* vg, View* view)
    {
        YGNodeRef node = view->getYGNode();
        Rect frame     = view->getFrame();
        float x = frame.getMinX(), y = frame.getMinY(), w = frame.getWidth(), h = frame.getHeight();

        float mt = YGNodeLayoutGetMargin(node, YGEdgeTop), mr = YGNodeLayoutGetMargin(node, YGEdgeRight);
        float mb = YGNodeLayoutGetMargin(node, YGEdgeBottom), ml = YGNodeLayoutGetMargin(node, YGEdgeLeft);
        float bt = YGNodeLayoutGetBorder(node, YGEdgeTop), br = YGNodeLayoutGetBorder(node, YGEdgeRight);
        float bb = YGNodeLayoutGetBorder(node, YGEdgeBottom), bl = YGNodeLayoutGetBorder(node, YGEdgeLeft);
        float pt = YGNodeLayoutGetPadding(node, YGEdgeTop), pr = YGNodeLayoutGetPadding(node, YGEdgeRight);
        float pb = YGNodeLayoutGetPadding(node, YGEdgeBottom), pl = YGNodeLayoutGetPadding(node, YGEdgeLeft);

        // As browsers' inspectors: margin orange, border yellow, padding green, content blue
        ring(vg, x - ml, y - mt, w + ml + mr, h + mt + mb, mt, mr, mb, ml, nvgRGBA(246, 160, 80, 110));
        ring(vg, x, y, w, h, bt, br, bb, bl, nvgRGBA(255, 220, 120, 120));
        ring(vg, x + bl, y + bt, w - bl - br, h - bt - bb, pt, pr, pb, pl, nvgRGBA(140, 200, 120, 110));
        nvgBeginPath(vg);
        nvgRect(vg, x + bl + pl, y + bt + pt, std::max(0.0f, w - bl - br - pl - pr), std::max(0.0f, h - bt - bb - pt - pb));
        nvgFillColor(vg, nvgRGBA(110, 170, 230, 90));
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRect(vg, x + 0.5f, y + 0.5f, std::max(0.0f, w - 1), std::max(0.0f, h - 1));
        nvgStrokeColor(vg, ACCENT);
        nvgStrokeWidth(vg, 1);
        nvgStroke(vg);

        // Its name and size on a tag above it (below when at the top)
        std::string tag = shortName(view) + "  " + number(w) + " x " + number(h);
        float bounds[4];
        nvgTextBounds(vg, 0, 0, tag.c_str(), nullptr, bounds);
        float tw = bounds[2] - bounds[0] + 8;
        float ty = y - mt - 18 >= 0 ? y - mt - 18 : y + h + mb + 2;
        nvgBeginPath(vg);
        nvgRect(vg, x, ty, tw, 16);
        nvgFillColor(vg, nvgRGBA(16, 18, 24, 220));
        nvgFill(vg);
        text(vg, x + 4, ty + 1, ACCENT, tag);
    }

    void outline(NVGcontext* vg, View* view, NVGcolor color)
    {
        Rect frame = view->getFrame();
        nvgBeginPath(vg);
        nvgRect(vg, frame.getMinX() - 1, frame.getMinY() - 1, frame.getWidth() + 2, frame.getHeight() + 2);
        nvgStrokeColor(vg, color);
        nvgStrokeWidth(vg, 2);
        nvgStroke(vg);
    }

    Sample average(float* maxFrame)
    {
        Sample sum;
        *maxFrame = 0;
        int count = std::min(historyCount, AVERAGE);
        for (int i = 0; i < count; i++)
        {
            const Sample& s = history[(historyEnd - 1 - i + HISTORY) % HISTORY];
            sum.frame += s.frame;
            sum.views += s.views;
            sum.layout += s.layout;
            sum.flush += s.flush;
            sum.present += s.present;
            sum.layouts += s.layouts;
            sum.draws += s.draws;
            sum.triangles += s.triangles;
            *maxFrame = std::max(*maxFrame, s.frame);
        }
        if (count > 0)
        {
            sum.frame /= count;
            sum.views /= count;
            sum.layout /= count;
            sum.flush /= count;
            sum.present /= count;
            sum.layouts   = (int)std::lround((float)sum.layouts / count);
            sum.draws     = (int)std::lround((float)sum.draws / count);
            sum.triangles = (int)std::lround((float)sum.triangles / count);
        }
        return sum;
    }

    void drawHeader(NVGcontext* vg, float x, float w)
    {
        float y = 8;
        nvgFontSize(vg, 14);
        float after = text(vg, x, y, ACCENT, "INSPECTOR");
        nvgFontSize(vg, 13);
        text(vg, after + 10, y + 1, DIM,
            Inspector::getMode() == Inspector::Mode::LIVE ? "live: follows the focus, the app keeps the input"
                                                          : "browse: the controller walks the tree");

        float maxFrame;
        Sample s = average(&maxFrame);
        y += 20;
        text(vg, x, y, TEXT,
            format("frame %.1f ms", s.frame) + format(" (max %.1f)", maxFrame) + "   "
                + std::to_string(Application::getFPS()) + " fps");
        y += LINE;
        text(vg, x, y, TEXT,
            format("views %.2f ms", s.views) + format("   layout %.2f ms", s.layout) + " (" + std::to_string(s.layouts)
                + ")" + format("   flush %.2f", s.flush) + format("   present %.1f", s.present));
        y += LINE;
        text(vg, x, y, TEXT,
            "nanovg " + std::to_string(s.draws) + " draw calls, " + std::to_string(s.triangles) + " triangles");

        // Frame times, the oldest on the left; the line is 60 fps
        y += LINE + 4;
        float gh = 24, bar = w / HISTORY;
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, gh);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 14));
        nvgFill(vg);
        for (int i = 0; i < historyCount; i++)
        {
            const Sample& sample = history[(historyEnd - historyCount + i + HISTORY) % HISTORY];
            float bh             = std::min(gh, sample.frame / 33.3f * gh);
            nvgBeginPath(vg);
            nvgRect(vg, x + (HISTORY - historyCount + i) * bar, y + gh - bh, std::max(1.0f, bar - 1), bh);
            nvgFillColor(vg, sample.frame > 17.5f ? nvgRGB(240, 110, 90) : nvgRGB(110, 200, 140));
            nvgFill(vg);
        }
        nvgBeginPath(vg);
        nvgRect(vg, x, y + gh / 2, w, 1);
        nvgFillColor(vg, DIM);
        nvgFill(vg);

        y += gh + 5;
        nvgFontSize(vg, 11);
        text(vg, x, y, DIM,
            Inspector::getMode() == Inspector::Mode::LIVE
                ? "F12 or L3+R3: browse the tree"
                : "Up/Down  Left parent  Right child  L/R page  A focus  Y focused  X side  B live");
        nvgFontSize(vg, 13);
    }

    void drawTree(NVGcontext* vg, float x, float w)
    {
        int index = rowOf(selected);
        if (index >= 0)
        {
            // Out of sight, it comes back to the middle
            if (index < scroll || index >= scroll + TREE_ROWS)
                scroll = index - TREE_ROWS / 2;
        }
        scroll = std::max(0, std::min(scroll, (int)rows.size() - TREE_ROWS));

        float y = TREE_TOP;
        nvgSave(vg);
        nvgIntersectScissor(vg, x - PAD, y, w + 2 * PAD, TREE_ROWS * ROW);
        for (int i = scroll; i < (int)rows.size() && i < scroll + TREE_ROWS; i++, y += ROW)
        {
            const Row& row = rows[i];
            if (!row.view)
            {
                text(vg, x, y + 1, ACCENT, row.heading);
                continue;
            }
            if (row.view == selected)
            {
                nvgBeginPath(vg);
                nvgRect(vg, x - PAD, y, w + 2 * PAD, ROW);
                nvgFillColor(vg, SELECTION);
                nvgFill(vg);
            }
            bool focused   = row.view == Application::getCurrentFocus();
            bool chain     = !focused && isInFocusChain(row.view);
            bool shown     = row.view->getVisibility() == Visibility::VISIBLE;
            NVGcolor color = focused ? FOCUS : (shown ? TEXT : DIM);

            float ix = x + std::min(row.depth, 20) * 10.0f;
            if (focused || chain)
                text(vg, ix - 9, y + 1, FOCUS, focused ? ">" : "|");
            float nx = text(vg, ix, y + 1, color, shortName(row.view));
            if (silent(row.view))
                text(vg, nx + 6, y + 1, WARN, "no a11y");

            std::string size = number(row.view->getWidth()) + " x " + number(row.view->getHeight());
            nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
            text(vg, x + w, y + 1, DIM, size);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        }
        nvgRestore(vg);

        // Where the rows shown are in the whole list
        if ((int)rows.size() > TREE_ROWS)
        {
            float track = TREE_ROWS * ROW;
            float thumb = std::max(12.0f, track * TREE_ROWS / rows.size());
            float top   = TREE_TOP + (track - thumb) * scroll / (rows.size() - TREE_ROWS);
            nvgBeginPath(vg);
            nvgRect(vg, x + w + PAD - 4, top, 3, thumb);
            nvgFillColor(vg, DIM);
            nvgFill(vg);
        }
    }

    void drawDetails(NVGcontext* vg, float x, float w, float top, float bottom)
    {
        float y = top;
        nvgBeginPath(vg);
        nvgRect(vg, x - PAD, y - 6, w + 2 * PAD, 1);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 40));
        nvgFill(vg);

        // The focus chain, at the bottom: from the root down to the focus
        std::string chain;
        std::vector<View*> chainViews;
        for (View* v = Application::getCurrentFocus(); v; v = v->hasParent() ? v->getParent() : nullptr)
            chainViews.push_back(v);
        for (auto it = chainViews.rbegin(); it != chainViews.rend(); ++it)
            chain += (chain.empty() ? "" : " > ") + shortName(*it);
        if (chain.empty())
            chain = "nothing focused";
        chain = "Focus: " + chain;
        float bounds[4];
        nvgFontSize(vg, 12);
        nvgTextBoxBounds(vg, 0, 0, w, chain.c_str(), nullptr, bounds);
        float chainHeight = std::min(bounds[3] - bounds[1], 5 * LINE);
        float chainTop    = bottom - chainHeight;
        nvgSave(vg);
        nvgIntersectScissor(vg, x, chainTop, w, chainHeight);
        nvgFillColor(vg, FOCUS);
        nvgTextBox(vg, x, chainTop, w, chain.c_str(), nullptr);
        nvgRestore(vg);
        nvgFontSize(vg, 13);
        bottom = chainTop - 6;

        if (!selected)
        {
            text(vg, x, y, DIM, "no view selected");
            return;
        }

        View* view     = selected;
        YGNodeRef node = view->getYGNode();
        text(vg, x, y, ACCENT, view->describe());
        y += LINE + 2;

        Rect frame = view->getFrame();
        text(vg, x, y, TEXT,
            "x " + number(frame.getMinX()) + "  y " + number(frame.getMinY()) + "   " + number(frame.getWidth()) + " x "
                + number(frame.getHeight()) + "   style " + yogaValue(YGNodeStyleGetWidth(node)) + " x "
                + yogaValue(YGNodeStyleGetHeight(node)));
        y += LINE;
        text(vg, x, y, TEXT,
            "margin " + edges(node, YGNodeLayoutGetMargin) + "   border " + edges(node, YGNodeLayoutGetBorder)
                + "   padding " + edges(node, YGNodeLayoutGetPadding));
        y += LINE;
        std::string flex = "grow " + number(YGNodeStyleGetFlexGrow(node)) + "  shrink "
            + number(YGNodeStyleGetFlexShrink(node)) + "  basis " + yogaValue(YGNodeStyleGetFlexBasis(node));
        if (dynamic_cast<Box*>(view))
        {
            YGFlexDirection direction = YGNodeStyleGetFlexDirection(node);
            flex += direction == YGFlexDirectionRow || direction == YGFlexDirectionRowReverse ? "   row" : "   column";
        }
        text(vg, x, y, TEXT, flex);
        y += LINE;

        const char* visibility = view->getVisibility() == Visibility::VISIBLE ? "visible"
            : view->getVisibility() == Visibility::INVISIBLE                  ? "invisible"
                                                                              : "gone";
        text(vg, x, y, TEXT,
            std::string(visibility) + format("   alpha %.2f", view->getAlpha()) + (view->isFocusable() ? "   focusable" : "")
                + (view->isFocused() ? "   focused" : "") + (view->isDetached() ? "   detached" : "")
                + (view->isCulled() ? "" : "   not culled"));
        y += LINE;

        // What the focus announcer says of it, and its role
        {
            std::string role        = accessibilityRoleName(view->getAccessibilityRole());
            std::string description = view->getAccessibilityDescription();
            float ax                = text(vg, x, y, TEXT, "a11y " + (role.empty() ? std::string("-") : role) + "  ");
            nvgSave(vg);
            nvgIntersectScissor(vg, ax, y, x + w - ax, LINE);
            if (silent(view))
                text(vg, ax, y, WARN, "focusable, no label: give it an accessibilityLabel");
            else
                text(vg, ax, y, DIM, description.empty() ? "nothing announced" : "\"" + description + "\"");
            nvgRestore(vg);
            y += LINE;
        }

        float sx = text(vg, x, y, TEXT, "background ");
        if (view->getBackground() == ViewBackground::SHAPE_COLOR)
        {
            swatch(vg, sx, y, view->getBackgroundColor());
            sx = text(vg, sx + 14, y, TEXT, hex(view->getBackgroundColor()));
            sx = themeKey(vg, sx, y, view, "backgroundColor");
        }
        else
        {
            const char* names[] = { "none", "sidebar", "backdrop", "colour", "vertical gradient", "horizontal gradient" };
            sx                  = text(vg, sx, y, TEXT, names[(int)view->getBackground()]);
        }
        if (view->getBorderThickness() > 0)
        {
            sx = text(vg, sx + 12, y, TEXT, "border " + number(view->getBorderThickness()) + " ");
            swatch(vg, sx, y, view->getBorderColor());
            sx = text(vg, sx + 14, y, TEXT, hex(view->getBorderColor()));
            sx = themeKey(vg, sx, y, view, "borderColor");
        }
        if (view->getCornerRadius() > 0)
            text(vg, sx + 12, y, TEXT, "radius " + number(view->getCornerRadius()));
        y += LINE;

        if (Label* label = dynamic_cast<Label*>(view))
        {
            std::string string = label->getFullText();
            if (string.size() > 48)
                string = string.substr(0, 45) + "...";
            float lx = text(vg, x, y, TEXT, "font " + number(label->getFontSize()) + "  ");
            swatch(vg, lx, y, label->getTextColor());
            lx = text(vg, lx + 14, y, TEXT, hex(label->getTextColor()));
            lx = themeKey(vg, lx, y, label, "textColor") + 8;
            nvgSave(vg);
            nvgIntersectScissor(vg, lx, y, x + w - lx, LINE);
            text(vg, lx, y, DIM, "\"" + string + "\"");
            nvgRestore(vg);
            y += LINE;
        }

        // What it looked up in the theme and the style while drawing itself
        y += 4;
        text(vg, x, y, DIM,
            "Theme while drawing: " + std::to_string(colors.size()) + " colours, " + std::to_string(metrics.size())
                + " metrics");
        y += LINE;
        nvgFontSize(vg, 12);
        float column = w;
        auto place   = [&](float* px, float* py) -> bool
        {
            *px = x;
            *py = y;
            y += LINE;
            return *py + LINE <= bottom;
        };
        for (auto& [name, color] : colors)
        {
            float px, py;
            if (!place(&px, &py))
                break;
            swatch(vg, px, py, color);
            nvgSave(vg);
            nvgIntersectScissor(vg, px + 14, py, column - 18, LINE);
            text(vg, px + 14, py, TEXT, name + " " + hex(color));
            nvgRestore(vg);
        }
        for (auto& [name, value] : metrics)
        {
            float px, py;
            if (!place(&px, &py))
                break;
            nvgSave(vg);
            nvgIntersectScissor(vg, px, py, column - 4, LINE);
            text(vg, px, py, TEXT, name + " = " + number(value));
            nvgRestore(vg);
        }
        nvgFontSize(vg, 13);
    }
} // namespace inspector_detail

using namespace inspector_detail;

void Inspector::setEnabled(bool on)
{
    Inspector::enabled = on;
    if (!on)
        Inspector::setMode(Mode::CLOSED);
}

void Inspector::setMode(Mode newMode)
{
    if (newMode != Mode::CLOSED && !Inspector::enabled)
        return;

    Mode old        = Inspector::mode;
    Inspector::mode = newMode;
    if (newMode == Mode::CLOSED)
    {
        selected                = nullptr;
        Inspector::recordTarget = nullptr;
        Inspector::recording    = false;
        rows.clear();
        historyCount = 0;
    }
    else if (old == Mode::CLOSED || newMode == Mode::LIVE)
    {
        selected = Application::getCurrentFocus();
    }
    std::memset(prev, 0, sizeof(prev));
    Application::requestRedraw();
}

void Inspector::processInput(InputManager* input, ControllerState* state)
{
    bool key    = input->getKeyboardKeyState(BRLS_KBD_KEY_F12);
    bool combo  = state->buttons[BUTTON_LSB] && state->buttons[BUTTON_RSB];
    bool toggle = (key && !keyHeld) || (combo && !comboHeld);
    keyHeld     = key;

    // The combo's buttons are the inspector's until both are let go
    if (combo)
        comboHeld = true;
    if (comboHeld)
    {
        if (!state->buttons[BUTTON_LSB] && !state->buttons[BUTTON_RSB])
            comboHeld = false;
        state->buttons[BUTTON_LSB] = false;
        state->buttons[BUTTON_RSB] = false;
    }

    if (toggle)
        setMode(mode == Mode::CLOSED ? Mode::LIVE : mode == Mode::LIVE ? Mode::BROWSE
                                                                       : Mode::CLOSED);

    if (mode != Mode::BROWSE)
        return;

    buildRows();
    if (!selected)
        selected = Application::getCurrentFocus();

    Time now     = getCPUTimeUsec();
    auto pressed = [&](int button)
    { return state->buttons[button] && !prev[button]; };
    auto repeats = [&](int button)
    {
        if (!state->buttons[button])
            return false;
        if (!prev[button])
        {
            nextRepeat[button] = now + 300000;
            return true;
        }
        if (now < nextRepeat[button])
            return false;
        nextRepeat[button] = now + 50000;
        return true;
    };

    if (repeats(BUTTON_NAV_UP))
        move(-1);
    if (repeats(BUTTON_NAV_DOWN))
        move(1);
    if (repeats(BUTTON_LB))
        move(-TREE_ROWS);
    if (repeats(BUTTON_RB))
        move(TREE_ROWS);
    if (pressed(BUTTON_NAV_LEFT) && selected && selected->hasParent())
        selected = selected->getParent();
    if (pressed(BUTTON_NAV_RIGHT) && selected)
    {
        Box* box = dynamic_cast<Box*>(selected);
        if (box && !box->getChildren().empty())
            selected = box->getChildren().front();
    }
    if (pressed(BUTTON_A) && selected)
        Application::giveFocus(selected);
    if (pressed(BUTTON_Y))
        selected = Application::getCurrentFocus();
    if (pressed(BUTTON_X))
        panelLeft = !panelLeft;
    if (pressed(BUTTON_B))
        setMode(Mode::LIVE);

    std::memcpy(prev, state->buttons, sizeof(prev));
    std::memset(state->buttons, 0, sizeof(state->buttons));
    for (float& axis : state->axes)
        axis = 0;
    Application::requestRedraw();
}

// The frames drawn while open, as the Profiler measured them
static void recordInspectedFrame()
{
    const FrameProfile& f = Profiler::getLastFrame();
    if (!Inspector::isOpen() || !f.drawn || f.interval == 0)
        return;
    Sample& s    = history[historyEnd];
    s.frame      = f.interval / 1000.0f;
    s.views      = f.draw / 1000.0f;
    s.layout     = f.layout / 1000.0f;
    s.layouts    = (int)f.layouts;
    s.flush      = f.flush / 1000.0f;
    s.present    = f.swap / 1000.0f;
    s.draws      = (int)f.drawCalls;
    s.triangles  = (int)f.triangles;
    historyEnd   = (historyEnd + 1) % HISTORY;
    historyCount = std::min(historyCount + 1, HISTORY);
}

void Inspector::beginFrame()
{
    static bool subscribed = false;
    if (!subscribed)
    {
        Profiler::getFrameEvent()->subscribe(recordInspectedFrame);
        subscribed = true;
    }

    colors.clear();
    metrics.clear();
    if (mode == Mode::LIVE || !selected)
        selected = Application::getCurrentFocus();
    Inspector::recordTarget = selected;
}

void Inspector::draw(FrameContext* ctx)
{
    NVGcontext* vg       = ctx->vg;
    Inspector::recording = false;

    buildRows();

    nvgSave(vg);
    nvgResetScissor(vg);
    nvgGlobalAlpha(vg, 1);
    nvgFontFaceId(vg, Application::getDefaultFont());
    nvgFontSize(vg, 13);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);

    View* focus = Application::getCurrentFocus();
    if (selected)
        drawBox(vg, selected);
    if (focus && focus != selected)
        outline(vg, focus, FOCUS);

    float px = panelLeft ? 0 : Application::contentWidth - PANEL_WIDTH;
    float h  = Application::contentHeight;
    nvgBeginPath(vg);
    nvgRect(vg, px, 0, PANEL_WIDTH, h);
    nvgFillColor(vg, PANEL);
    nvgFill(vg);
    nvgIntersectScissor(vg, px, 0, PANEL_WIDTH, h);

    float x = px + PAD, w = PANEL_WIDTH - 2 * PAD;
    drawHeader(vg, x, w);
    drawTree(vg, x, w);
    drawDetails(vg, x, w, TREE_TOP + TREE_ROWS * ROW + 10, h - 8);

    nvgRestore(vg);

    // Its figures move with every frame
    Application::requestRedraw();
}

void Inspector::forgetView(View* view)
{
    if (selected == view)
        selected = nullptr;
    if (Inspector::recordTarget == view)
    {
        Inspector::recordTarget = nullptr;
        Inspector::recording    = false;
    }
}

void Inspector::recordColor(const std::string& name, NVGcolor color)
{
    for (auto& entry : colors)
        if (entry.first == name)
            return;
    colors.emplace_back(name, color);
}

void Inspector::recordMetric(const std::string& name, float value)
{
    for (auto& entry : metrics)
        if (entry.first == name)
            return;
    metrics.emplace_back(name, value);
}

} // namespace brls
