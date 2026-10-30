// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <math.h>
#ifndef _MSC_VER
#include <cxxabi.h>
#endif
#include <tinyxml2.h>
#include <yoga/Yoga.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <sstream>
#include <aurore/core/animation.hpp>
#include <aurore/core/application.hpp>
#include <aurore/core/box.hpp>
#include <aurore/core/hot_reload.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/input.hpp>
#include <aurore/core/inspector.hpp>
#include <aurore/core/profiler.hpp>
#include <aurore/core/theme_decor.hpp>
#include <aurore/core/util.hpp>
#include <aurore/core/view.hpp>
#include <aurore/views/applet_frame.hpp>

// Avoid conflicts with macro definitions in windows.h
#undef RGB
#undef TRANSPARENT
#undef RELATIVE
#undef ABSOLUTE

using namespace aurore::literals;

namespace aurore
{

void AppletFrameItem::setHintView(View* hintView)
{
    this->hintView = hintView;
    this->hintView->ptrLock();
}

AppletFrameItem::~AppletFrameItem()
{
    if (hintView)
    {
        if (!hintView->isPtrLocked())
            delete hintView;
        else
            hintView->freeView();
    }
}

View::View()
{
    // Instantiate and prepare YGNode
    this->ygNode = YGNodeNewWithConfig(Application::getYogaConfig());
    YGNodeSetContext(this->ygNode, this);

    YGNodeStyleSetWidthAuto(this->ygNode);
    YGNodeStyleSetHeightAuto(this->ygNode);

    // The common XML attributes: registered when first needed
    // (ensureCommonAttributes()).

    // Default values
    Style style = Application::getStyle();

    this->highlightCornerRadius = style["aurore/highlight/corner_radius"];
}

static int shakeAnimation(float t, float a) // a = amplitude
{
    // Damped sine wave
    float w = 0.8f; // period
    float c = 0.35f; // damp factor
    return roundf(a * exp(-(c * t)) * sin(w * t));
}

void View::shakeHighlight(FocusDirection direction)
{
    this->highlightShaking        = true;
    this->highlightShakeStart     = getCPUTimeUsec() / 1000;
    this->highlightShakeDirection = direction;
    this->highlightShakeAmplitude = std::rand() % 15 + 10;
}

float View::getAlpha(bool child)
{
    return this->alpha * (this->parent ? this->parent->getAlpha(true) : 1.0f);
}

NVGcolor View::a(NVGcolor color)
{
    NVGcolor newColor = color; // copy
    newColor.a *= this->getAlpha();
    return newColor;
}

NVGpaint View::a(NVGpaint paint)
{
    NVGpaint newPaint = paint; // copy
    newPaint.innerColor.a *= this->getAlpha();
    newPaint.outerColor.a *= this->getAlpha();
    return newPaint;
}

const std::vector<GestureRecognizer*>& View::getGestureRecognizers()
{
    return this->gestureRecognizers;
}

void View::interruptGestures(bool onlyIfUnsureState)
{
    for (GestureRecognizer* recognizer : getGestureRecognizers())
        recognizer->interrupt(onlyIfUnsureState);

    if (parent)
        parent->interruptGestures(onlyIfUnsureState);
}

bool View::removeGestureRecognizer(GestureRecognizer* recognizer)
{
    auto it = std::find(this->gestureRecognizers.begin(), this->gestureRecognizers.end(), recognizer);
    if (it == this->gestureRecognizers.end())
        return false;
    this->gestureRecognizers.erase(it);
    delete recognizer;
    return true;
}

void View::addGestureRecognizer(GestureRecognizer* recognizer)
{
    this->gestureRecognizers.push_back(recognizer);
}

Sound View::gestureRecognizerRequest(TouchState touch, MouseState mouse, View* firstResponder)
{
    Sound soundToPlay = touch.phase == TouchPhase::START ? SOUND_TOUCH : SOUND_NONE;

    for (GestureRecognizer* recognizer : getGestureRecognizers())
    {
        if (!recognizer->isEnabled())
            continue;

        GestureState state = recognizer->recognitionLoop(touch, mouse, this, &soundToPlay);
        if (state == GestureState::START || state == GestureState::END)
            firstResponder->interruptGestures(true);
    }

    Sound parentSound = SOUND_NONE;
    if (parent)
        parentSound = parent->gestureRecognizerRequest(touch, mouse, firstResponder);

    if (soundToPlay == SOUND_NONE)
        soundToPlay = parentSound;

    return soundToPlay;
}

void View::frame(FrameContext* ctx)
{
    if (this->visibility != Visibility::VISIBLE)
        return;

    // The inspector notes what its selected view, and not its children,
    // looks up in the theme and the style
    View* const inspected = Inspector::recordTarget;
    bool wasRecording     = false;
    if (inspected)
    {
        wasRecording         = Inspector::recording;
        Inspector::recording = inspected == this;
    }

    // State styles changed since the last frame: applied after the focus
    // and press handlers have all run, whatever their order.
    this->applyPendingStyles();

    Style style    = Application::getStyle();
    Theme oldTheme = ctx->theme;

    nvgSave(ctx->vg);

    // Theme override
    if (this->themeOverride)
        ctx->theme = *themeOverride;

    Rect frame   = getFrame();
    float x      = frame.getMinX();
    float y      = frame.getMinY();
    float width  = frame.getWidth();
    float height = frame.getHeight();

    this->applyScale(ctx->vg, frame);

    if (this->alpha > 0.0f && this->collapseState != 0.0f)
    {
        // A translucent dialog over a theme's picture: the picture under it,
        // dimmed as around it, not the screen behind it
        if (this->themeArea == ThemeArea::DIALOG && this->backgroundColor.a < 1.0f && Application::hasThemeBackground())
            this->drawThemeUnderlay(ctx, frame);

        // Draw background
        this->drawBackground(ctx->vg, ctx, style, frame);

        // The theme's picture of this area, over its surface
        if (this->themeArea != ThemeArea::NONE)
            this->drawThemeBackdrop(ctx, frame);

        // Draw shadow
        if (this->shadowType != ShadowType::NONE && (this->showShadow || Application::getInputType() == InputType::TOUCH))
            this->drawShadow(ctx->vg, ctx, style, frame);

        // Draw border
        if (this->borderThickness > 0.0f)
            this->drawBorder(ctx->vg, ctx, style, frame);

        this->drawLine(ctx, frame);

        // Draw highlight background
        if (this->highlightAlpha > 0.0f && !this->hideHighlightBackground && !this->hideHighlight)
            this->drawHighlight(ctx->vg, ctx->theme, this->highlightAlpha, style, true);

        // Draw click animation
        if (this->clickAlpha > 0.0f)
            this->drawClickAnimation(ctx->vg, ctx, frame);

        // Collapse clipping
        if (this->collapseState < 1.0f || this->clipsToBounds)
        {
            nvgSave(ctx->vg);
            nvgIntersectScissor(ctx->vg, x, y, width, height * this->collapseState);
        }

        // Draw the view
        this->draw(ctx->vg, x, y, width, height, style, ctx);

        if (this->wireframeEnabled)
            this->drawWireframe(ctx, frame);

        // Reset clipping
        if (this->collapseState < 1.0f || this->clipsToBounds)
            nvgRestore(ctx->vg);
    }

    // Cleanup
    if (this->themeOverride)
        ctx->theme = oldTheme;

    nvgRestore(ctx->vg);

    if (inspected)
        Inspector::recording = wasRecording;
}

void View::drawThemeUnderlay(FrameContext* ctx, const Rect& frame)
{
    NVGcontext* vg = ctx->vg;
    Application::drawThemeBackground(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), this->cornerRadius);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), this->cornerRadius);
    nvgFillColor(vg, ctx->theme["aurore/backdrop"]);
    nvgFill(vg);
}

void View::drawThemeBackdrop(FrameContext* ctx, const Rect& frame)
{
    if (!ThemeDecorations::hasPicture(this->themeArea))
        return;
    Rect area = frame;
    if (this->themeArea == ThemeArea::HEADER)
    {
        // A screen's header, from one side of the screen to the other
        View* screen = this->hasParent() ? this->getParent() : nullptr;
        if (!screen || screen->hasParent())
            return;
        area = Rect(screen->getX(), frame.getMinY(), screen->getWidth(), frame.getHeight());
    }
    ThemeDecorations::drawBackdrop(this->themeArea, ctx->vg, area.getMinX(), area.getMinY(), area.getWidth(), area.getHeight(),
        this->themeArea == ThemeArea::HEADER ? 0.0f : this->cornerRadius);
}

void View::frameHighlight(FrameContext* ctx)
{
    if (this->alpha > 0.0f && this->collapseState != 0.0f && this->highlightAlpha > 0.0f && !this->hideHighlightBorder && !this->hideHighlight)
    {
        nvgSave(ctx->vg);
        this->applyScale(ctx->vg, this->getFrame());
        this->drawHighlight(ctx->vg, ctx->theme, this->highlightAlpha, Application::getStyle(), false);
        nvgRestore(ctx->vg);
    }
}

void View::applyScale(NVGcontext* vg, const Rect& frame)
{
    if (this->scaleX == 1.0f && this->scaleY == 1.0f)
        return;

    // Around the view's centre
    float cx = frame.getMidX();
    float cy = frame.getMidY();
    nvgTranslate(vg, cx, cy);
    nvgScale(vg, this->scaleX, this->scaleY);
    nvgTranslate(vg, -cx, -cy);
}

void View::setScale(float scale)
{
    if (this->transitionStyle("scale", StyleValue::of(scale)))
        return;
    this->scaleX = scale;
    this->scaleY = scale;
}

void View::setScaleX(float scale)
{
    this->scaleX = scale;
}

void View::setScaleY(float scale)
{
    this->scaleY = scale;
}

void View::resetClickAnimation()
{
    this->clickAlpha.stop();
}

void View::playClickAnimation(bool reverse, bool animateBack, bool force)
{
    // A pressed: style: released when the animation plays back, or after the
    // time it would take when there is none to play back.
    this->notePress(!reverse, !reverse && animateBack && Application::getInputType() != InputType::TOUCH && hideClickAnimation && !force);

    if (hideClickAnimation && !force)
        return;

    this->resetClickAnimation();

    Style style = Application::getStyle();

    this->clickAlpha.reset(reverse ? 1.0f : 0.0f);

    this->clickAlpha.addStep(
        reverse ? 0.0f : 1.0f,
        style["aurore/animations/highlight"],
        reverse ? EasingFunction::quadraticOut : EasingFunction::quadraticIn);

    this->clickAlpha.setEndCallback([this, reverse, animateBack](bool finished)
        {
        if (reverse || !animateBack || Application::getInputType() == InputType::TOUCH)
            return;

        this->playClickAnimation(true); });

    this->clickAlpha.start();
}

std::string View::getClassString() const
{
    // Taken from: https://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname/4541470#4541470
    const char* name = typeid(*this).name();
#ifndef _MSC_VER
    int status = 0;
    std::unique_ptr<char, void (*)(void*)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };
    return (status == 0) ? res.get() : name;
#else
    return name;
#endif
}

std::string View::describe() const
{
    std::string classString = this->getClassString();

    if (this->id != "")
        return classString + " (id=\"" + this->id + "\")";

    return classString;
}

YGNode* View::getYGNode()
{
    return this->ygNode;
}

const std::vector<std::shared_ptr<Action>>& View::getActions()
{
    return this->actions;
}

void View::drawClickAnimation(NVGcontext* vg, FrameContext* ctx, Rect frame)
{
    Theme theme    = ctx->theme;
    NVGcolor color = theme["aurore/click_pulse"];

    color.a *= this->clickAlpha;

    nvgFillColor(vg, a(color));
    nvgBeginPath(vg);

    bool hasRadii = (this->cornerRadii[0] != 0 || this->cornerRadii[1] != 0 || this->cornerRadii[2] != 0 || this->cornerRadii[3] != 0);
    if (hasRadii)
        nvgRoundedRectVarying(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), this->cornerRadii[0], this->cornerRadii[1], this->cornerRadii[2], this->cornerRadii[3]);
    else
        nvgRoundedRect(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), 4);

    nvgFill(vg);
}

void View::drawLine(FrameContext* ctx, Rect frame)
{
    // Don't setup and draw empty nvg path if there is no line to draw
    if (this->lineTop <= 0 && this->lineRight <= 0 && this->lineBottom <= 0 && this->lineLeft <= 0)
        return;

    nvgBeginPath(ctx->vg);
    nvgFillColor(ctx->vg, a(this->lineColor));

    if (this->lineTop > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), frame.size.width, this->lineTop);

    if (this->lineRight > 0)
        nvgRect(ctx->vg, frame.getMaxX(), frame.getMinY(), this->lineRight, frame.size.height);

    if (this->lineBottom > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMaxY() - this->lineBottom, frame.size.width, this->lineBottom);

    if (this->lineLeft > 0)
        nvgRect(ctx->vg, frame.getMinX() - this->lineLeft, frame.getMinY(), this->lineLeft, frame.size.height);

    nvgFill(ctx->vg);
}

void View::drawWireframe(FrameContext* ctx, Rect frame)
{
    nvgStrokeWidth(ctx->vg, 1);

    // Outline
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(0, 0, 255));
    nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight());
    nvgStroke(ctx->vg);

    if (this->hasParent())
    {
        // Diagonals
        nvgStrokeColor(ctx->vg, nvgRGB(0, 0, 255));
        nvgStrokeWidth(ctx->vg, 1);

        nvgBeginPath(ctx->vg);
        nvgMoveTo(ctx->vg, frame.getMinX(), frame.getMinY());
        nvgLineTo(ctx->vg, frame.getMaxX(), frame.getMaxY());
        nvgStroke(ctx->vg);

        nvgBeginPath(ctx->vg);
        nvgMoveTo(ctx->vg, frame.getMaxX(), frame.getMinY());
        nvgLineTo(ctx->vg, frame.getMinX(), frame.getMaxY());
        nvgStroke(ctx->vg);
    }

    // Padding
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(0, 255, 0));

    float paddingTop    = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeTop));
    float paddingLeft   = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeLeft));
    float paddingBottom = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeBottom));
    float paddingRight  = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeRight));

    // Top
    if (paddingTop > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), paddingTop);

    // Right
    if (paddingRight > 0)
        nvgRect(ctx->vg, frame.getMaxX() - paddingRight, frame.getMinY(), paddingRight, frame.getHeight());

    // Bottom
    if (paddingBottom > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMaxY() - paddingBottom, frame.getWidth(), paddingBottom);

    // Left
    if (paddingLeft > 0)
        nvgRect(ctx->vg, frame.getMinX(), frame.getMinY(), paddingLeft, frame.getHeight());

    nvgStroke(ctx->vg);

    // Margins
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(255, 0, 0));

    float marginTop    = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeTop));
    float marginLeft   = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeLeft));
    float marginBottom = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeBottom));
    float marginRight  = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeRight));

    // Top
    if (marginTop > 0)
        nvgRect(ctx->vg, frame.getMinX() - marginLeft, frame.getMinY() - marginTop, frame.getWidth() + marginLeft + marginRight, marginTop);

    // Right
    if (marginRight > 0)
        nvgRect(ctx->vg, frame.getMaxX(), frame.getMinY() - marginTop, marginRight, frame.getHeight() + marginTop + marginBottom);

    // Bottom
    if (marginBottom > 0)
        nvgRect(ctx->vg, frame.getMinX() - marginLeft, frame.getMaxY(), frame.getWidth() + marginLeft + marginRight, marginBottom);

    // Left
    if (marginLeft > 0)
        nvgRect(ctx->vg, frame.getMinX() - marginLeft, frame.getMinY() - marginTop, marginLeft, frame.getHeight() + marginTop + marginBottom);

    nvgStroke(ctx->vg);
}

void View::drawBorder(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame)
{
    nvgBeginPath(vg);
    nvgStrokeColor(vg, a(this->borderColor));
    nvgStrokeWidth(vg, this->borderThickness);

    bool hasRadii = (this->cornerRadii[0] != 0 || this->cornerRadii[1] != 0 || this->cornerRadii[2] != 0 || this->cornerRadii[3] != 0);
    if (hasRadii)
        nvgRoundedRectVarying(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), this->cornerRadii[0], this->cornerRadii[1], this->cornerRadii[2], this->cornerRadii[3]);
    else
        nvgRoundedRect(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), this->cornerRadius);

    nvgStroke(vg);
}

// A shadow's colour (a theme key's, usually black), `opacity` from 0 to 255
// times the key's own alpha
NVGcolor View::shadowColor(NVGcolor color, float opacity)
{
    auto channel = [](float value)
    { return (unsigned)std::lround(value * 255.0f); };
    return RGBA(channel(color.r), channel(color.g), channel(color.b), (unsigned)(opacity * color.a));
}

void View::drawShadow(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame)
{
    float shadowWidth   = 0.0f;
    float shadowFeather = 0.0f;
    float shadowOpacity = 0.0f;
    float shadowOffset  = 0.0f;

    switch (this->shadowType)
    {
        case ShadowType::GENERIC:
            shadowWidth   = style["aurore/shadow/width"];
            shadowFeather = style["aurore/shadow/feather"];
            shadowOpacity = style["aurore/shadow/opacity"];
            shadowOffset  = style["aurore/shadow/offset"];
            break;
        case ShadowType::CUSTOM:
            break;
        case ShadowType::NONE:
            break;
    }

    float maxRadius      = std::max({ this->cornerRadii[0], this->cornerRadii[1], this->cornerRadii[2], this->cornerRadii[3] });
    NVGpaint shadowPaint = nvgBoxGradient(
        vg,
        frame.getMinX(), frame.getMinY() + shadowWidth,
        frame.getWidth(), frame.getHeight(),
        maxRadius * 2, shadowFeather,
        this->shadowColor(ctx->theme["aurore/shadow"], shadowOpacity * alpha), TRANSPARENT);

    nvgBeginPath(vg);
    nvgRect(
        vg,
        frame.getMinX() - shadowOffset,
        frame.getMinY() - shadowOffset,
        frame.getWidth() + shadowOffset * 2,
        frame.getHeight() + shadowOffset * 3);
    nvgRoundedRectVarying(vg, frame.getMinX(), frame.getMinY(), frame.getWidth(), frame.getHeight(), this->cornerRadii[0], this->cornerRadii[1], this->cornerRadii[2], this->cornerRadii[3]);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadowPaint);
    nvgFill(vg);
}

void View::collapse(bool animated)
{
    if (animated)
    {
        Style style = Application::getStyle();

        this->collapseState.reset();

        this->collapseState.addStep(0.0f, style["aurore/animations/collapse"], EasingFunction::quadraticOut);

        this->collapseState.setTickCallback([this]
            {
            if (this->hasParent())
                this->getParent()->invalidate(); });

        this->collapseState.start();
    }
    else
    {
        this->collapseState = 0.0f;
    }
}

bool View::isCollapsed()
{
    return this->collapseState < 1.0f;
}

void View::expand(bool animated)
{
    if (animated)
    {
        Style style = Application::getStyle();

        this->collapseState.reset();

        this->collapseState.addStep(1.0f, style["aurore/animations/collapse"], EasingFunction::quadraticOut);

        this->collapseState.setTickCallback([this]
            {
            if (this->hasParent())
                this->getParent()->invalidate(); });

        this->collapseState.start();
    }
    else
    {
        this->collapseState = 1.0f;
    }
}

void View::setAlpha(float alpha)
{
    if (this->transitionStyle("alpha", StyleValue::of(alpha)))
        return;
    this->alpha = alpha;
}

// The highlight's border glides from the view that had it to the next one.
// Only the rectangle drawn last is kept, never the view (it may be gone).
static bool highlightGlide          = true;
static const View* glideLastView    = nullptr;
static float glideLast[4]           = { 0, 0, 0, 0 };
static float glideFrom[4]           = { 0, 0, 0, 0 };
static uint64_t glideLastFrame      = 0; // the frame it was drawn on
static Time glideStart              = 0;
static bool gliding                 = false;
static constexpr Time GLIDE_TIME_MS = 140;

void View::setHighlightGlide(bool glide)
{
    highlightGlide = glide;
    gliding        = false;
}

void View::drawHighlight(NVGcontext* vg, Theme theme, float alpha, Style style, bool background)
{
    if (Application::getInputType() == InputType::TOUCH)
        return;

    nvgSave(vg);
    nvgResetScissor(vg);

    float padding      = this->highlightPadding;
    float cornerRadius = this->highlightCornerRadius;
    float strokeWidth  = style["aurore/highlight/stroke_width"];

    float x      = this->getX() - padding - strokeWidth / 2;
    float y      = this->getY() - padding - strokeWidth / 2;
    float width  = this->getWidth() + padding * 2 + strokeWidth;
    float height = this->getHeight() + padding * 2 + strokeWidth;

    if (!background)
    {
        Time now = getCPUTimeUsec() / 1000;
        if (this != glideLastView)
        {
            // From where the border was drawn last, if that was on the frame
            // drawn before (not after a while in touch mode, or on another
            // screen; a frame skipped by redraw on demand does not count)
            gliding = highlightGlide && !Application::isReduceMotionSet() && glideLastView
                && glideLastFrame + 1 >= Application::getDrawnFrameCount();
            if (gliding)
            {
                std::copy(std::begin(glideLast), std::end(glideLast), std::begin(glideFrom));
                glideStart = now;
            }
            glideLastView = this;
        }

        if (gliding)
        {
            float t = (float)(now - glideStart) / (float)GLIDE_TIME_MS;
            Application::requestRedraw();
            if (t >= 1.0f)
            {
                gliding = false;
            }
            else
            {
                float e = 1.0f - (1.0f - t) * (1.0f - t); // quadratic out
                x       = glideFrom[0] + (x - glideFrom[0]) * e;
                y       = glideFrom[1] + (y - glideFrom[1]) * e;
                width   = glideFrom[2] + (width - glideFrom[2]) * e;
                height  = glideFrom[3] + (height - glideFrom[3]) * e;
            }
        }

        glideLast[0]   = x;
        glideLast[1]   = y;
        glideLast[2]   = width;
        glideLast[3]   = height;
        glideLastFrame = Application::getDrawnFrameCount();
    }

    // Shake animation
    if (this->highlightShaking)
    {
        Time curTime = getCPUTimeUsec() / 1000;
        Time t       = (curTime - highlightShakeStart) / 10;
        Application::requestRedraw();

        if (t >= style["aurore/animations/highlight_shake"])
        {
            this->highlightShaking = false;
        }
        else
        {
            switch (this->highlightShakeDirection)
            {
                case FocusDirection::RIGHT:
                    x += shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
                case FocusDirection::LEFT:
                    x -= shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
                case FocusDirection::DOWN:
                    y += shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
                case FocusDirection::UP:
                    y -= shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
            }
        }
    }

    // Draw
    if (background)
    {
        // Background
        NVGcolor highlightBackgroundColor = theme["aurore/highlight/background"];
        nvgFillColor(vg, RGBAf(highlightBackgroundColor.r, highlightBackgroundColor.g, highlightBackgroundColor.b, this->highlightAlpha));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgFill(vg);
    }
    else
    {
#ifdef SIMPLE_HIGHLIGHT
        // Border
        nvgBeginPath(vg);
        nvgStrokeColor(vg, a(theme["aurore/highlight/color1"]));
        nvgStrokeWidth(vg, style["aurore/highlight/stroke_width"]);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);
#else
        float shadowOffset = style["aurore/highlight/shadow_offset"];

        // Shadow
        NVGpaint shadowPaint = nvgBoxGradient(vg,
            x, y + style["aurore/highlight/shadow_width"],
            width, height,
            cornerRadius * 2, style["aurore/highlight/shadow_feather"],
            this->shadowColor(theme["aurore/highlight/shadow"], style["aurore/highlight/shadow_opacity"] * alpha), TRANSPARENT);

        nvgBeginPath(vg);
        nvgRect(vg, x - shadowOffset, y - shadowOffset,
            width + shadowOffset * 2, height + shadowOffset * 3);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadowPaint);
        nvgFill(vg);

        // Border
        float gradientX, gradientY, color;
        getHighlightAnimation(&gradientX, &gradientY, &color);

        // The border pulses between the two highlight colours. (Since the
        // 2.0 rewrite both terms used color1, so it never pulsed.)
        NVGcolor highlightColor1 = theme["aurore/highlight/color1"];
        NVGcolor highlightColor2 = theme["aurore/highlight/color2"];

        NVGcolor pulsationColor = RGBAf((color * highlightColor1.r) + (1 - color) * highlightColor2.r,
            (color * highlightColor1.g) + (1 - color) * highlightColor2.g,
            (color * highlightColor1.b) + (1 - color) * highlightColor2.b,
            alpha);

        NVGcolor borderColor = theme["aurore/highlight/color2"];
        borderColor.a        = 0.5f * alpha * this->getAlpha();

        float strokeWidth = style["aurore/highlight/stroke_width"];

        NVGpaint border1Paint = nvgRadialGradient(vg,
            x + gradientX * width, y + gradientY * height,
            strokeWidth * 10, strokeWidth * 40,
            borderColor, TRANSPARENT);

        NVGpaint border2Paint = nvgRadialGradient(vg,
            x + (1 - gradientX) * width, y + (1 - gradientY) * height,
            strokeWidth * 10, strokeWidth * 40,
            borderColor, TRANSPARENT);

        nvgBeginPath(vg);
        nvgStrokeColor(vg, pulsationColor);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgStrokePaint(vg, border1Paint);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgStrokePaint(vg, border2Paint);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);
#endif
    }

    nvgRestore(vg);
}

void View::setBackground(ViewBackground background)
{
    this->background = background;
}

void View::setBackgroundLinearGradient(NVGcolor start, NVGcolor end, ViewBackground direction)
{
    if (direction != ViewBackground::HORIZONTAL_LINEAR && direction != ViewBackground::VERTICAL_LINEAR)
        fatal("setBackgroundLinearGradient: direction must be VERTICAL_LINEAR or HORIZONTAL_LINEAR");

    this->backgroundStartColor = start;
    this->backgroundEndColor   = end;

    this->setBackground(direction);
}

void View::drawBackground(NVGcontext* vg, FrameContext* ctx, Style style, Rect frame)
{
    float x      = frame.getMinX();
    float y      = frame.getMinY();
    float width  = frame.getWidth();
    float height = frame.getHeight();

    Theme theme = ctx->theme;

    switch (this->background)
    {
        case ViewBackground::SIDEBAR:
        {
            float backdropHeight  = style["aurore/sidebar/border_height"];
            NVGcolor sidebarColor = theme["aurore/sidebar/background"];

            // Solid color
            nvgBeginPath(vg);
            nvgFillColor(vg, a(sidebarColor));
            nvgRect(vg, x, y + backdropHeight, width, height - backdropHeight * 2);
            nvgFill(vg);

            // Borders gradient
            //  Top
            NVGpaint topGradient = nvgLinearGradient(vg, x, y + backdropHeight, x, y, a(sidebarColor), TRANSPARENT);
            nvgBeginPath(vg);
            nvgFillPaint(vg, topGradient);
            nvgRect(vg, x, y, width, backdropHeight);
            nvgFill(vg);

            // Bottom
            NVGpaint bottomGradient = nvgLinearGradient(vg, x, y + height - backdropHeight, x, y + height, a(sidebarColor), TRANSPARENT);
            nvgBeginPath(vg);
            nvgFillPaint(vg, bottomGradient);
            nvgRect(vg, x, y + height - backdropHeight, width, backdropHeight);
            nvgFill(vg);
            break;
        }
        case ViewBackground::VERTICAL_LINEAR:
        case ViewBackground::HORIZONTAL_LINEAR:
        {
            bool vertical     = this->background == ViewBackground::VERTICAL_LINEAR;
            NVGpaint gradient = nvgLinearGradient(vg, x, y, vertical ? x : x + width, vertical ? y + height : y, a(backgroundStartColor), a(backgroundEndColor));
            nvgBeginPath(vg);
            nvgFillPaint(vg, gradient);
            if (std::all_of(this->backgroundRadius.begin(), this->backgroundRadius.end(), [](float i)
                    { return i == 0.0f; }))
                nvgRect(vg, x, y, width, height);
            else
                nvgRoundedRectVarying(vg, x, y, width, height, backgroundRadius[0], backgroundRadius[1], backgroundRadius[2], backgroundRadius[3]);
            nvgFill(vg);
            break;
        }
        case ViewBackground::BACKDROP:
        {
            nvgFillColor(vg, a(theme["aurore/backdrop"]));
            nvgBeginPath(vg);
            nvgRect(vg, x, y, width, height);
            nvgFill(vg);
            break;
        }
        case ViewBackground::SHAPE_COLOR:
        {
            nvgFillColor(vg, a(this->backgroundColor));
            nvgBeginPath(vg);

            bool hasRadii = (this->cornerRadii[0] != 0 || this->cornerRadii[1] != 0 || this->cornerRadii[2] != 0 || this->cornerRadii[3] != 0);
            if (hasRadii)
                nvgRoundedRectVarying(vg, x, y, width, height, this->cornerRadii[0], this->cornerRadii[1], this->cornerRadii[2], this->cornerRadii[3]);
            else
                nvgRect(vg, x, y, width, height);

            nvgFill(vg);
            break;
        }
        case ViewBackground::NONE:
            break;
    }
}

template <class ButtonType>
View::ActionIterator View::getAction(ButtonType button)
{
    return std::find_if(this->actions.begin(), this->actions.end(), [button](const std::shared_ptr<Action>& action)
        { return *action == button; });
}

View::ActionIterator View::getAction(ActionIdentifier identifier)
{
    return std::find_if(this->actions.begin(), this->actions.end(), [identifier](const std::shared_ptr<Action>& action)
        { return action->getIdentifier() == identifier; });
}

template <class ButtonType, class ActionType>
ActionIdentifier View::registerAction(const std::string& hintText, ButtonType button, const ActionListener& actionListener, bool hidden, bool allowRepeating, enum Sound sound)
{
    ActionIdentifier nextIdentifier = (this->actions.size() == 0) ? 1 : this->actions.back()->getIdentifier() + 1;
    auto newAction                  = std::make_shared<ActionType>(button, nextIdentifier, hintText, true, hidden, allowRepeating, sound, actionListener);

    auto it = getAction(button);
    if (it != this->actions.end())
    {
        *it = newAction;
    }
    else
    {
        this->actions.push_back(newAction);
    }

    return nextIdentifier;
}

ActionIdentifier View::registerAction(const std::string& hintText, const ControllerButton button, const ActionListener& actionListener, const bool hidden, const bool allowRepeating, const enum Sound sound)
{
    return registerAction<ControllerButton, GamepadAction>(hintText, button, actionListener, hidden, allowRepeating, sound);
}

ActionIdentifier View::registerAction(const AuroreKeyCombination key, const ActionListener& actionListener, const bool allowRepeating)
{
    Application::addToWatchedKeys(key);
    return registerAction<AuroreKeyCode, KeyboardAction>("", key, actionListener, true, allowRepeating, SOUND_NONE);
}

void View::unregisterAction(ActionIdentifier identifier)
{
    if (const auto it = getAction(identifier); it != this->actions.end())
    {
        if ((*it)->getType() == ACTION_KEYBOARD)
        {
            Application::removeWatchedKeys(AuroreKeyCombination { (*it)->getButton() });
        }
        this->actions.erase(it);
    }
}

void View::registerClickAction(const ActionListener& actionListener)
{
    this->registerAction("hints/ok"_i18n, BUTTON_A, actionListener, false, false, SOUND_CLICK);
}

void View::updateActionHint(const enum ControllerButton button, const std::string& hintText)
{
    if (const auto it = getAction(button); it != this->actions.end())
        (*it)->setHintText(hintText);
}

void View::setActionAvailable(const enum ControllerButton button, const bool available)
{
    if (const auto it = getAction(button); it != this->actions.end())
        (*it)->setAvailable(available);

    Application::getGlobalHintsUpdateEvent()->fire();
}

void View::setActionHidden(const enum ControllerButton button, const bool hidden)
{
    if (const auto it = getAction(button); it != this->actions.end())
        (*it)->setHidden(hidden);

    Application::getGlobalHintsUpdateEvent()->fire();
}

void View::setActionsAvailable(const bool available) const
{
    for (const auto& action : this->actions)
        action->setAvailable(available);

    Application::getGlobalHintsUpdateEvent()->fire();
}

void View::setParent(Box* parent, void* parentUserdata)
{
    if (this->parentUserdata)
        free(this->parentUserdata);

    this->parent         = parent;
    this->parentUserdata = parentUserdata;
}

void* View::getParentUserData()
{
    return this->parentUserdata;
}

bool View::isFocused()
{
    return this->focused;
}

enum Sound View::getFocusSound()
{
    return this->focusSound;
}

Box* View::getParent()
{
    return this->parent;
}

bool View::hasParent()
{
    return this->parent;
}

void View::setDimensions(float width, float height)
{
    YGNodeStyleSetMinWidthPercent(this->ygNode, 0);
    YGNodeStyleSetMinHeightPercent(this->ygNode, 0);

    if (width == View::AUTO)
    {
        YGNodeStyleSetWidthAuto(this->ygNode);
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetWidth(this->ygNode, width);
        YGNodeStyleSetMinWidth(this->ygNode, width);
    }

    if (height == View::AUTO)
    {
        YGNodeStyleSetHeightAuto(this->ygNode);
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetHeight(this->ygNode, height);
        YGNodeStyleSetMinHeight(this->ygNode, height);
    }

    this->invalidate();
}

void View::setWidth(float width)
{
    // The same width: nothing to lay out again (invalidate() lays the whole
    // screen out). Set as below: the width and the minimum in points, or
    // both automatic.
    const YGValue current = YGNodeStyleGetWidth(this->ygNode);
    const YGValue minimum = YGNodeStyleGetMinWidth(this->ygNode);
    if (width == View::AUTO ? current.unit == YGUnitAuto && minimum.unit == YGUnitUndefined
                            : current.unit == YGUnitPoint && current.value == width && minimum.unit == YGUnitPoint && minimum.value == width)
        return;

    YGNodeStyleSetMinWidthPercent(this->ygNode, 0);

    if (width == View::AUTO)
    {
        YGNodeStyleSetWidthAuto(this->ygNode);
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetWidth(this->ygNode, width);
        YGNodeStyleSetMinWidth(this->ygNode, width);
    }

    this->invalidate();
}

void View::setHeight(float height)
{
    // The same height: nothing to lay out again (invalidate() lays the whole
    // screen out). Set as below: the height and the minimum in points, or
    // both automatic.
    const YGValue current = YGNodeStyleGetHeight(this->ygNode);
    const YGValue minimum = YGNodeStyleGetMinHeight(this->ygNode);
    if (height == View::AUTO ? current.unit == YGUnitAuto && minimum.unit == YGUnitUndefined
                             : current.unit == YGUnitPoint && current.value == height && minimum.unit == YGUnitPoint && minimum.value == height)
        return;

    YGNodeStyleSetMinHeightPercent(this->ygNode, 0);

    if (height == View::AUTO)
    {
        YGNodeStyleSetHeightAuto(this->ygNode);
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetHeight(this->ygNode, height);
        YGNodeStyleSetMinHeight(this->ygNode, height);
    }

    this->invalidate();
}

void View::setSize(Size size)
{
    setWidth(size.width);
    setHeight(size.height);
}

void View::setWidthPercentage(float percentage)
{
    YGNodeStyleSetWidthPercent(this->ygNode, percentage);
    YGNodeStyleSetMinWidthPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setHeightPercentage(float percentage)
{
    YGNodeStyleSetHeightPercent(this->ygNode, percentage);
    YGNodeStyleSetMinHeightPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMinWidth(float minWidth)
{
    if (minWidth == View::AUTO)
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMinWidth(this->ygNode, minWidth);

    this->invalidate();
}

void View::setMinWidthPercentage(float percentage)
{
    YGNodeStyleSetMinWidthPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMinHeight(float minHeight)
{
    if (minHeight == View::AUTO)
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMinHeight(this->ygNode, minHeight);

    this->invalidate();
}

void View::setMinHeightPercentage(float percentage)
{
    YGNodeStyleSetMinHeightPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMaxWidth(float maxWidth)
{
    if (maxWidth == View::AUTO)
        YGNodeStyleSetMaxWidth(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMaxWidth(this->ygNode, maxWidth);

    this->invalidate();
}

void View::setMaxWidthPercentage(float percentage)
{
    YGNodeStyleSetMaxWidthPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMaxHeight(float maxHeight)
{
    if (maxHeight == View::AUTO)
        YGNodeStyleSetMaxHeight(this->ygNode, YGUndefined);
    else
        YGNodeStyleSetMaxHeight(this->ygNode, maxHeight);

    this->invalidate();
}

void View::setMaxHeightPercentage(float percentage)
{
    YGNodeStyleSetMaxHeightPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMarginTop(float top)
{
    if (top == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeTop);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeTop, top);

    this->invalidate();
}

void View::setMarginRight(float right)
{
    if (right == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeRight);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeRight, right);

    this->invalidate();
}

float View::getMarginRight()
{
    return ntz(YGNodeStyleGetMargin(this->ygNode, YGEdgeRight).value);
}

float View::getMarginLeft()
{
    return ntz(YGNodeStyleGetMargin(this->ygNode, YGEdgeLeft).value);
}

void View::setMarginBottom(float bottom)
{
    if (bottom == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeBottom);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeBottom, bottom);

    this->invalidate();
}

void View::setMarginLeft(float left)
{
    if (left == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeLeft);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void View::setMargins(float top, float right, float bottom, float left)
{
    if (top == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeTop);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeTop, top);

    if (right == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeRight);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeRight, right);

    if (bottom == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeBottom);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeBottom, bottom);

    if (left == aurore::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeLeft);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void View::setPositionTop(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeTop, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeTop, pos);
    this->invalidate();
}

void View::setPositionRight(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeRight, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeRight, pos);
    this->invalidate();
}

void View::setPositionBottom(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeBottom, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeBottom, pos);
    this->invalidate();
}

void View::setPositionLeft(float pos)
{
    if (pos == View::AUTO)
        YGNodeStyleSetPosition(this->ygNode, YGEdgeLeft, YGUndefined);
    else
        YGNodeStyleSetPosition(this->ygNode, YGEdgeLeft, pos);
    this->invalidate();
}

void View::setPositionTopPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeTop, percentage);
    this->invalidate();
}

void View::setPositionRightPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeRight, percentage);
    this->invalidate();
}

void View::setPositionBottomPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeBottom, percentage);
    this->invalidate();
}

void View::setPositionLeftPercentage(float percentage)
{
    YGNodeStyleSetPositionPercent(this->ygNode, YGEdgeLeft, percentage);
    this->invalidate();
}

void View::setPositionType(PositionType type)
{
    switch (type)
    {
        case PositionType::RELATIVE:
            YGNodeStyleSetPositionType(this->ygNode, YGPositionTypeRelative);
            break;
        case PositionType::ABSOLUTE:
            YGNodeStyleSetPositionType(this->ygNode, YGPositionTypeAbsolute);
            break;
        case PositionType::STATIC:
            YGNodeStyleSetPositionType(this->ygNode, YGPositionTypeStatic);
            break;
    }

    this->invalidate();
}

void View::setBoxSizing(BoxSizing sizing)
{
    YGNodeStyleSetBoxSizing(this->ygNode, sizing == BoxSizing::CONTENT_BOX ? YGBoxSizingContentBox : YGBoxSizingBorderBox);
    this->invalidate();
}

void View::setGrow(float grow)
{
    YGNodeStyleSetFlexGrow(this->ygNode, grow);
    this->invalidate();
}

void View::setShrink(float shrink)
{
    YGNodeStyleSetFlexShrink(this->ygNode, shrink);
    this->invalidate();
}

void View::setAlignSelf(AlignSelf alignSelf)
{
    switch (alignSelf)
    {
        case AlignSelf::AUTO:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignAuto);
            break;
        case AlignSelf::FLEX_START:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignFlexStart);
            break;
        case AlignSelf::CENTER:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignCenter);
            break;
        case AlignSelf::FLEX_END:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignFlexEnd);
            break;
        case AlignSelf::STRETCH:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignStretch);
            break;
        case AlignSelf::BASELINE:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignBaseline);
            break;
        case AlignSelf::SPACE_BETWEEN:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignSpaceBetween);
            break;
        case AlignSelf::SPACE_AROUND:
            YGNodeStyleSetAlignSelf(this->ygNode, YGAlignSpaceAround);
            break;
    }

    this->invalidate();
}

void View::invalidate()
{
    Application::requestRedraw();

    if (YGNodeHasMeasureFunc(this->ygNode))
        YGNodeMarkDirty(this->ygNode);

    if (this->hasParent() && !this->detached)
    {
        this->getParent()->invalidate();
    }
    else if (Profiler::isMeasuring())
    {
        Time start = getRealTimeUsec();
        YGNodeCalculateLayout(this->ygNode, YGUndefined, YGUndefined, YGDirectionLTR);
        Profiler::addLayout(getRealTimeUsec() - start);
    }
    else
    {
        YGNodeCalculateLayout(this->ygNode, YGUndefined, YGUndefined, YGDirectionLTR);
    }
}

Rect View::getFrame()
{
    return Rect(getX(), getY(), getWidth(), getHeight());
}

float View::getX()
{
    if (this->hasParent())
        return this->getParent()->getX() + YGNodeLayoutGetLeft(this->ygNode) + this->translation.x + (isDetached() ? this->detachedOrigin.x : 0);
    return YGNodeLayoutGetLeft(this->ygNode) + this->translation.x;
}

float View::getY()
{
    if (this->hasParent())
        return this->getParent()->getY() + YGNodeLayoutGetTop(this->ygNode) + this->translation.y + (isDetached() ? this->detachedOrigin.y : 0);
    return YGNodeLayoutGetTop(this->ygNode) + this->translation.y;
}

Rect View::getLocalFrame()
{
    return Rect(getLocalX(), getLocalY(), getWidth(), getHeight());
}

float View::getLocalX()
{
    return YGNodeLayoutGetLeft(this->ygNode) + this->translation.x + (isDetached() ? this->detachedOrigin.x : 0);
}

float View::getLocalY()
{
    return YGNodeLayoutGetTop(this->ygNode) + this->translation.y + (isDetached() ? this->detachedOrigin.y : 0);
}

float View::getHeight(bool includeCollapse)
{
    return YGNodeLayoutGetHeight(this->ygNode) * (includeCollapse ? this->collapseState.getValue() : 1.0f);
}

float View::getWidth()
{
    return YGNodeLayoutGetWidth(this->ygNode);
}

void View::detach()
{
    this->detached = true;
}

void View::setDetachedPosition(float x, float y)
{
    this->detachedOrigin.x = x;
    this->detachedOrigin.y = y;
}

void View::setDetachedPositionX(float x)
{
    this->detachedOrigin.x = x;
}

void View::setDetachedPositionY(float y)
{
    this->detachedOrigin.y = y;
}

bool View::isDetached()
{
    return this->detached;
}

void View::onFocusGained()
{
    this->focused = true;
    this->noteStyleState(StyleState::FOCUS, true);

    Style style = Application::getStyle();

    this->highlightAlpha.reset();
    this->highlightAlpha.addStep(1.0f, style["aurore/animations/highlight"], EasingFunction::quadraticOut);
    this->highlightAlpha.start();

    this->focusEvent.fire(this);

    if (this->hasParent())
        this->getParent()->onChildFocusGained(this, this);
}

GenericEvent* View::getFocusEvent()
{
    return &this->focusEvent;
}

GenericEvent* View::getFocusLostEvent()
{
    return &this->focusLostEvent;
}

void View::onFocusLost()
{
    this->focused = false;
    this->noteStyleState(StyleState::FOCUS, false);

    Style style = Application::getStyle();

    this->highlightAlpha.reset();
    this->highlightAlpha.addStep(0.0f, style["aurore/animations/highlight"], EasingFunction::quadraticOut);
    this->highlightAlpha.start();

    this->focusLostEvent.fire(this);

    if (this->hasParent())
        this->getParent()->onChildFocusLost(this, this);
}

void View::setInFadeAnimation(bool inFadeAnimation)
{
    this->inFadeAnimation = inFadeAnimation;
}

void View::removeFromSuperView(bool free)
{
    if (parent)
        parent->removeView(this, free);
}

bool View::isTranslucent()
{
    return this->fadeIn || this->inFadeAnimation;
}

void View::show(std::function<void(void)> cb)
{
    this->show(cb, true, this->getShowAnimationDuration(TransitionAnimation::FADE));
}

void View::show(std::function<void(void)> cb, bool animate, float animationDuration)
{
    if (!this->hidden)
    {
        this->onShowAnimationEnd();
        if (cb)
            cb();
        return;
    }

    aurore::Logger::debug("Showing {}", this->describe());

    this->hidden = false;

    this->fadeIn = true;

    if (animate)
    {
        this->alpha.reset(0.0f);

        this->alpha.addStep(1.0f, animationDuration, EasingFunction::quadraticOut);

        this->alpha.setEndCallback([this, cb](bool finished)
            {
            this->fadeIn = false;
            this->onShowAnimationEnd();
            if (cb)
                cb(); });

        this->alpha.start();
    }
    else
    {
        this->alpha  = 1.0f;
        this->fadeIn = false;
        this->onShowAnimationEnd();
        if (cb)
            cb();
    }
}

void View::hide(std::function<void(void)> cb)
{
    this->hide(cb, true, this->getShowAnimationDuration(TransitionAnimation::FADE));
}

void View::hide(std::function<void(void)> cb, bool animated, float animationDuration)
{
    if (this->hidden)
    {
        if (cb)
            cb();
        return;
    }

    aurore::Logger::debug("Hiding {}", this->describe());

    this->hidden = true;
    this->fadeIn = false;

    if (animated)
    {
        this->alpha.reset(1.0f);

        this->alpha.addStep(0.0f, animationDuration, EasingFunction::quadraticOut);

        this->alpha.setEndCallback([cb](bool finished)
            {
            if (finished && cb) cb(); });

        this->alpha.start();
    }
    else
    {
        this->alpha = 0.0f;
        if (cb)
            cb();
    }
}

float View::getShowAnimationDuration(TransitionAnimation animation)
{
    Style style = Application::getStyle();

    if (animation == TransitionAnimation::SLIDE_LEFT || animation == TransitionAnimation::SLIDE_RIGHT)
        fatal("Slide animation is not supported on views");

    return style["aurore/animations/show"];
}

bool View::isHidden()
{
    return this->hidden;
}

void View::overrideTheme(Theme* newTheme)
{
    this->themeOverride = newTheme;
}

void View::setAspectRatio(float value)
{
    if (value <= 0)
        return;
    this->aspectRatio = value;
    YGNodeStyleSetAspectRatio(this->ygNode, value);
    this->invalidate();
}

float View::getAspectRatio()
{
    return this->aspectRatio;
}

void View::onParentFocusGained(View* focusedView)
{
}

void View::onParentFocusLost(View* focusedView)
{
}

View* View::getNextFocus(FocusDirection direction, View* currentView)
{
    return getParent()->getNextFocus(direction, currentView);
}

void View::setCustomNavigationRoute(FocusDirection direction, View* target)
{
    if (!this->focusable)
        fatal("Only focusable views can have a custom navigation route");

    this->customFocusByPtr[direction] = target;
}

void View::setCustomNavigationRoute(FocusDirection direction, std::string targetId)
{
    if (!this->focusable)
        fatal("Only focusable views can have a custom navigation route");

    this->customFocusById[direction] = targetId;
}

bool View::hasCustomNavigationRouteByPtr(FocusDirection direction)
{
    return this->customFocusByPtr.contains(direction);
}

bool View::hasCustomNavigationRouteById(FocusDirection direction)
{
    return this->customFocusById.contains(direction);
}

View* View::getCustomNavigationRoutePtr(FocusDirection direction)
{
    return this->customFocusByPtr[direction];
}

std::string View::getCustomNavigationRouteId(FocusDirection direction)
{
    return this->customFocusById[direction];
}

View::~View()
{
    this->resetClickAnimation();

    // Parent userdata
    if (this->parentUserdata)
    {
        free(this->parentUserdata);
        this->parentUserdata = nullptr;
    }

    for (const auto& action : this->actions)
    {
        if (action->getType() == ACTION_KEYBOARD)
        {
            Application::removeWatchedKeys(AuroreKeyCombination { action->getButton() });
        }
    }

    // Focus sanity check
    Application::forgetView(this);

    Application::tryDeinitFirstResponder(this);
    for (GestureRecognizer* recognizer : this->gestureRecognizers)
        delete recognizer;

    alpha.stop();
    clickAlpha.stop();
    highlightAlpha.stop();
    collapseState.stop();

    if (this->topScreenView)
        this->topScreenView->freeView();

    YGNodeFree(this->ygNode);

    alive.reset();
}

void View::setTopScreenView(View* view)
{
    if (this->topScreenView)
        this->topScreenView->freeView();
    this->topScreenView = view;
    Application::requestRedraw();
}

std::string View::getStringXMLAttributeValue(std::string value)
{
    if (startsWith(value, "@i18n/"))
    {
        std::string stringName = value.substr(6);
        return getStr(stringName);
    }

    return value;
}

std::string View::getFilePathXMLAttributeValue(std::string value)
{
    if (startsWith(value, "@res/"))
    {
        // A resource stays in the resources (as in Theme::resolveAsset)
        if (!isResourceNameSafe(value))
        {
            Logger::warning("{}: \"..\" is not allowed in a resource path", escapeControlChars(value));
            return "";
        }
        std::string resPath = value.substr(5);
#ifdef USE_LIBROMFS
        return resPath;
#else
        return std::string(AURORE_RESOURCES) + resPath;
#endif
    }

    return value;
}

void View::ensureCommonAttributes()
{
    if (this->commonAttributesRegistered)
        return;
    this->commonAttributesRegistered  = true;
    this->registeringCommonAttributes = true;
    this->registerCommonAttributes();
    this->registeringCommonAttributes = false;
}

// "focus:backgroundColor" -> FOCUS, "backgroundColor"
static bool splitStateAttribute(const std::string& name, StyleState* state, std::string* property)
{
    size_t colon = name.find(':');
    if (colon == std::string::npos)
        return false;
    static const std::unordered_map<std::string, StyleState> STATES = {
        { "hover", StyleState::HOVER },
        { "focus", StyleState::FOCUS },
        { "pressed", StyleState::PRESSED },
        { "disabled", StyleState::DISABLED },
    };
    auto it = STATES.find(name.substr(0, colon));
    if (it == STATES.end())
        return false;
    *state    = it->second;
    *property = name.substr(colon + 1);
    return true;
}

// `compact:width` -> COMPACT and `width`: an attribute for one class of
// screen width (Application::getWidthClass)
static bool splitWidthAttribute(const std::string& name, WidthClass* widthClass, std::string* attribute)
{
    size_t colon = name.find(':');
    if (colon == std::string::npos)
        return false;
    static const std::unordered_map<std::string, WidthClass> CLASSES = {
        { "compact", WidthClass::COMPACT },
        { "medium", WidthClass::MEDIUM },
        { "expanded", WidthClass::EXPANDED },
    };
    auto it = CLASSES.find(name.substr(0, colon));
    if (it == CLASSES.end())
        return false;
    *widthClass = it->second;
    *attribute  = name.substr(colon + 1);
    return true;
}

// std::stof(), a value past a float's range read as not a number (which
// the callers refuse) instead of an exception they do not expect
static float readXMLFloat(const std::string& text)
{
    try
    {
        return std::stof(text);
    }
    catch (const std::out_of_range&)
    {
        return NAN;
    }
}

bool View::applyXMLAttribute(std::string name, std::string value)
{
    this->ensureCommonAttributes();

    // For one class of width: applied on such a screen, checked on the others
    WidthClass widthClass;
    std::string attribute;
    if (splitWidthAttribute(name, &widthClass, &attribute))
    {
        if (widthClass == Application::getWidthClass())
            return this->applyXMLAttribute(attribute, value);
        return this->isXMLAttributeValid(attribute);
    }

    StyleState state;
    std::string property;
    if (splitStateAttribute(name, &state, &property))
        return this->setStateStyle(state, property, value);

    // String -> string
    if (this->stringAttributes.contains(name))
    {
        if (startsWith(value, "@i18n/"))
        {
            this->stringAttributes[name](View::getStringXMLAttributeValue(value));
            return true;
        }

        this->stringAttributes[name](value);
        return true;
    }

    // File path -> file path
    if (startsWith(value, "@res/"))
    {
        if (this->filePathAttributes.contains(name))
        {
            // "" when refused: the attribute is left unset
            std::string path = View::getFilePathXMLAttributeValue(value);
            if (!path.empty())
#ifdef USE_LIBROMFS
                this->filePathAttributes[name](value);
#else
                this->filePathAttributes[name](path);
#endif
            return true;
        }
        else
        {
            return false; // unknown res
        }
    }
    else
    {
        if (this->filePathAttributes.contains(name))
        {
            this->filePathAttributes[name](value);
            return true;
        }

        // don't return false as it can be anything else
    }

    // Auto -> auto
    if (value == "auto")
    {
        if (this->autoAttributes.contains(name))
        {
            this->autoAttributes[name]();
            return true;
        }
        else
        {
            return false;
        }
    }

    // Ends with dp -> float at the density in use
    if (endsWith(value, "dp"))
    {
        try
        {
            float floatValue = readXMLFloat(value.substr(0, value.length() - 2));
            if (!std::isfinite(floatValue) || !this->floatAttributes.contains(name))
                return false;
            this->floatAttributes[name](dp(floatValue));
            return true;
        }
        catch (const std::invalid_argument& exception)
        {
            return false;
        }
    }

    // Ends with px -> float
    if (endsWith(value, "px"))
    {
        // Strip the px and parse the float value
        std::string newFloat = value.substr(0, value.length() - 2);
        try
        {
            float floatValue = readXMLFloat(newFloat);
            if (!std::isfinite(floatValue))
                return false;
            if (this->floatAttributes.contains(name))
            {
                this->floatAttributes[name](floatValue);
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (const std::invalid_argument& exception)
        {
            return false;
        }
    }

    // Ends with % -> percentage
    if (endsWith(value, "%"))
    {
        // Strip the % and parse the float value
        std::string newFloat = value.substr(0, value.length() - 1);
        try
        {
            float floatValue = readXMLFloat(newFloat);

            if (!std::isfinite(floatValue) || floatValue < -100 || floatValue > 100)
                return false;

            if (this->percentageAttributes.contains(name))
            {
                this->percentageAttributes[name](floatValue);
                return true;
            }
            else
            {
                return false;
            }
        }
        catch (const std::invalid_argument& exception)
        {
            return false;
        }
    }
    // Starts with @style -> float
    else if (startsWith(value, "@style/"))
    {
        // Parse the style name
        std::string styleName = value.substr(7); // length of "@style/"
        float value           = Application::getStyle()[styleName]; // will throw logic_error if the metric doesn't exist

        if (this->floatAttributes.contains(name))
        {
            this->floatAttributes[name](value);
            return true;
        }
        else
        {
            return false;
        }
    }
    // Starts with # -> color
    else if (startsWith(value, "#"))
    {
        NVGcolor color;
        if (!Theme::parseColor(value, &color) || !this->colorAttributes.contains(name))
            return false;
        this->colorAttributes[name](color);
        return true;
    }
    // Starts with @theme -> a theme colour, kept by key when the attribute
    // is a style property (it follows the theme)
    else if (startsWith(value, "@theme/"))
    {
        std::string key = value.substr(7); // length of "@theme/"
        StyleValue current;
        if (this->getStyleProperty(name, &current) && current.type == StyleValue::Type::COLOR)
            return this->setThemeColor(name, key);

        if (!this->colorAttributes.contains(name) || !Application::getTheme().hasColor(key))
            return false;
        this->colorAttributes[name](Application::getTheme()[key]);
        return true;
    }
    // Equals true or false -> bool
    else if (value == "true" || value == "false")
    {
        bool boolValue = value == "true" ? true : false;

        if (this->boolAttributes.contains(name))
        {
            this->boolAttributes[name](boolValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    // Valid float -> float, otherwise unknown attribute
    try
    {
        float newValue = readXMLFloat(value);
        if (!std::isfinite(newValue))
            return false;
        if (this->floatAttributes.contains(name))
        {
            this->floatAttributes[name](newValue);
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (const std::invalid_argument& exception)
    {
        return false;
    }
}

// The file or resource each document read comes from, for the errors
static std::unordered_map<const tinyxml2::XMLDocument*, std::string>& xmlDocumentNames()
{
    static std::unordered_map<const tinyxml2::XMLDocument*, std::string> names;
    return names;
}

void nameXMLDocument(const tinyxml2::XMLDocument* document, const std::string& name)
{
    if (!name.empty())
        xmlDocumentNames()[document] = name;
}

// The element being read, for fatalXML()
static const tinyxml2::XMLElement* currentXMLElement = nullptr;

ReadingXMLElement::ReadingXMLElement(const tinyxml2::XMLElement* element)
    : previous(currentXMLElement)
{
    currentXMLElement = element;
}

ReadingXMLElement::~ReadingXMLElement()
{
    currentXMLElement = previous;
}

std::string View::describeXMLLocation(const tinyxml2::XMLElement* element)
{
    std::string line = std::to_string(element->GetLineNum());
    auto it          = xmlDocumentNames().find(element->GetDocument());
    if (it == xmlDocumentNames().end())
        return "line " + line;
    return it->second + ":" + line;
}

void View::fatalXML(const std::string& message)
{
    if (!currentXMLElement)
        fatal(message);
    fatal(describeXMLLocation(currentXMLElement) + ": <" + currentXMLElement->Name() + ">: " + message);
}

std::string View::closestXMLName(const std::string& name, const std::vector<std::string>& known)
{
    // Levenshtein's distance, case aside
    auto distance = [](const std::string& a, const std::string& b)
    {
        std::vector<size_t> row(b.size() + 1);
        for (size_t j = 0; j <= b.size(); j++)
            row[j] = j;
        for (size_t i = 1; i <= a.size(); i++)
        {
            size_t diagonal = row[0];
            row[0]          = i;
            for (size_t j = 1; j <= b.size(); j++)
            {
                size_t above = row[j];
                bool same    = std::tolower((unsigned char)a[i - 1]) == std::tolower((unsigned char)b[j - 1]);
                row[j]       = std::min({ row[j] + 1, row[j - 1] + 1, diagonal + (same ? 0 : 1) });
                diagonal     = above;
            }
        }
        return row[b.size()];
    };
    std::string best;
    size_t bestDistance = std::max<size_t>(1, std::min<size_t>(3, name.size() / 3)) + 1;
    for (const std::string& candidate : known)
    {
        size_t d = distance(name, candidate);
        if (d < bestDistance)
        {
            best         = candidate;
            bestDistance = d;
        }
    }
    return best;
}

void View::applyXMLAttributes(tinyxml2::XMLElement* element)
{
    if (!element)
        return;
    ReadingXMLElement reading(element);

    // The attributes for a class of width last, over the others whatever
    // their order (compact:width="200" beats width="400")
    WidthClass widthClass;
    std::string unused;
    for (bool widthPass : { false, true })
        for (const tinyxml2::XMLAttribute* attribute = element->FirstAttribute(); attribute != nullptr; attribute = attribute->Next())
        {
            std::string name  = attribute->Name();
            std::string value = std::string(attribute->Value());
            if (splitWidthAttribute(name, &widthClass, &unused) != widthPass)
                continue;

            if (!this->applyXMLAttribute(name, value))
                this->printXMLAttributeErrorMessage(element, name, value);
        }
}

bool View::isXMLAttributeValid(std::string attributeName)
{
    this->ensureCommonAttributes();
    WidthClass widthClass;
    std::string attribute;
    if (splitWidthAttribute(attributeName, &widthClass, &attribute))
        return this->isXMLAttributeValid(attribute);
    StyleState state;
    std::string property;
    StyleValue probe;
    if (splitStateAttribute(attributeName, &state, &property))
        return this->getStyleProperty(property, &probe);
    return this->knownAttributes.contains(attributeName);
}

// A custom resource (a folder the app's users may change): an error in it is
// logged, and the built-in resource is used instead
static View* createFromCustomXMLFile(const std::string& path)
{
    try
    {
        return View::createFromXMLFile(path);
    }
    catch (const std::exception& e)
    {
        Logger::error("{}; the built-in resource is used instead", e.what());
        return nullptr;
    }
}

View* View::createFromXMLResource(std::string name)
{
    std::string disk = HotReload::diskResource("@res/xml/" + name);
    if (!disk.empty())
        return View::createFromXMLFile(disk);

    // Check if custom xml file exists
    if (!View::CUSTOM_RESOURCES_PATH.empty() && std::ifstream { View::CUSTOM_RESOURCES_PATH + "xml/" + name }.good())
    {
        if (View* view = createFromCustomXMLFile(View::CUSTOM_RESOURCES_PATH + "xml/" + name))
            return view;
    }

#ifdef USE_LIBROMFS
    View::currentXMLResource = "xml/" + name;
    View* view               = View::createFromXMLString(romfs::get("xml/" + name).string());
    View::currentXMLResource.clear();
    return view;
#else
    return View::createFromXMLFile(std::string(AURORE_RESOURCES) + "xml/" + name);
#endif
}

View* View::createFromXMLString(std::string_view xml)
{
    std::shared_ptr<tinyxml2::XMLDocument> document = getXMLCache(xml);
    tinyxml2::XMLElement* element                   = document->RootElement();

    if (!element)
    {
        // A string_view need not end with a NUL: its size bounds the parse
        tinyxml2::XMLError error = document->Parse(xml.data(), xml.size());

        if (error != tinyxml2::XMLError::XML_SUCCESS)
            fatal("Invalid XML when creating View from XML: " + View::describeXMLError(document.get()));

        element = document->RootElement();

        if (!element)
            fatal("Invalid XML: no element found");
    }
    nameXMLDocument(document.get(), View::currentXMLResource);

    View* view = View::createFromXMLElement(element);
    return view;
}

View* View::createFromXMLFile(std::string path)
{
    HotReload::noteXMLFile(path);
    std::shared_ptr<tinyxml2::XMLDocument> document = getXMLCache(path);
    tinyxml2::XMLElement* element                   = document->RootElement();

    if (!element)
    {
        tinyxml2::XMLError error = document->LoadFile(path.c_str());

        if (error != tinyxml2::XMLError::XML_SUCCESS)
            fatal("Unable to load XML file: " + View::describeXMLError(document.get(), path));

        element = document->RootElement();

        if (!element)
            fatal("Unable to load XML file \"" + path + "\": no root element found, is the file empty?");
    }
    nameXMLDocument(document.get(), path);

    View* view = View::createFromXMLElement(element);
    return view;
}

// Elements read inside one another, aurore:View files included: a file that
// includes itself would recurse until the stack runs out
static constexpr int MAX_XML_DEPTH = 64;
static int xmlDepth                = 0;

namespace
{
    struct XMLDepthGuard
    {
        XMLDepthGuard()
        {
            if (xmlDepth >= MAX_XML_DEPTH)
                fatal("XML nested more than " + std::to_string(MAX_XML_DEPTH) + " levels deep (a aurore:View including itself?)");
            xmlDepth++;
        }
        ~XMLDepthGuard() { xmlDepth--; }
    };
} // namespace

View* View::createFromXMLElement(tinyxml2::XMLElement* element)
{
    XMLDepthGuard depth;

    if (!element)
        return nullptr;

    std::string viewName = element->Name();
    ReadingXMLElement reading(element);

    // Instantiate the view
    View* view = nullptr;

    // Special case where element name is aurore:View: create from given XML file.
    // XML attributes are explicitely not passed down to the created view.
    // To create a custom view from XML that you can reuse in other XML files,
    // make a class inheriting aurore::Box and use the inflateFromXML* methods.
    if (viewName == "aurore:View")
    {
        const tinyxml2::XMLAttribute* xmlAttribute = element->FindAttribute("xml");

        if (xmlAttribute)
        {
            std::string disk = HotReload::diskResource(xmlAttribute->Value());
            if (!disk.empty())
                view = View::createFromXMLFile(disk);
            else
#ifdef USE_LIBROMFS
            {
                std::string resource     = View::getFilePathXMLAttributeValue(xmlAttribute->Value());
                std::string outer        = View::currentXMLResource;
                View::currentXMLResource = resource;
                view                     = View::createFromXMLString(romfs::get(resource).string());
                View::currentXMLResource = outer;
            }
#else
                view = View::createFromXMLFile(View::getFilePathXMLAttributeValue(xmlAttribute->Value()));
#endif
        }
        else
        {
            fatalXML("aurore:View XML tag must have an \"xml\" attribute");
        }
    }
    // Otherwise look in the register
    else
    {
        if (!Application::XMLViewsRegisterContains(viewName))
        {
            std::string closest = closestXMLName(viewName, Application::getXMLViewNames());
            fatalXML("Unknown XML tag \"" + viewName + "\"" + (closest.empty() ? "" : "; did you mean \"" + closest + "\"?"));
        }

        view = Application::getXMLViewCreator(viewName)();

        view->applyXMLAttributes(element);
    }

    unsigned count = 0;
    unsigned max   = view->getMaximumAllowedXMLElements();
    for (tinyxml2::XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
    {
        if (count >= max)
            fatalXML("View \"" + view->describe() + "\" is only allowed to have " + std::to_string(max) + " children XML elements");
        else
            view->handleXMLElement(child);

        count++;
    }

    return view;
}

void View::handleXMLElement(tinyxml2::XMLElement* element)
{
    ReadingXMLElement reading(element);
    fatalXML("This view cannot have child XML tags");
}

void View::setMaximumAllowedXMLElements(unsigned max)
{
    this->maximumAllowedXMLElements = max;
}

unsigned View::getMaximumAllowedXMLElements()
{
    return this->maximumAllowedXMLElements;
}

void View::registerCommonAttributes()
{
    // Width
    this->registerAutoXMLAttribute("width", [this]
        { this->setWidth(View::AUTO); });

    this->registerFloatXMLAttribute("width", [this](float value)
        { this->setWidth(value); });

    this->registerPercentageXMLAttribute("width", [this](float value)
        { this->setWidthPercentage(value); });

    // Height
    this->registerAutoXMLAttribute("height", [this]
        { this->setHeight(View::AUTO); });

    this->registerFloatXMLAttribute("height", [this](float value)
        { this->setHeight(value); });

    this->registerPercentageXMLAttribute("height", [this](float value)
        { this->setHeightPercentage(value); });

    // Min width
    this->registerAutoXMLAttribute("minWidth", [this]
        { this->setMinWidth(View::AUTO); });

    this->registerFloatXMLAttribute("minWidth", [this](float value)
        { this->setMinWidth(value); });

    this->registerPercentageXMLAttribute("minWidth", [this](float percentage)
        { this->setMinWidthPercentage(percentage); });

    // Min height
    this->registerAutoXMLAttribute("minHeight", [this]
        { this->setMinHeight(View::AUTO); });

    this->registerFloatXMLAttribute("minHeight", [this](float value)
        { this->setMinHeight(value); });

    this->registerPercentageXMLAttribute("minHeight", [this](float percentage)
        { this->setMinHeightPercentage(percentage); });

    // Max width
    this->registerAutoXMLAttribute("maxWidth", [this]
        { this->setMaxWidth(View::AUTO); });

    this->registerFloatXMLAttribute("maxWidth", [this](float value)
        { this->setMaxWidth(value); });

    this->registerPercentageXMLAttribute("maxWidth", [this](float percentage)
        { this->setMaxWidthPercentage(percentage); });

    // Max height
    this->registerAutoXMLAttribute("maxHeight", [this]
        { this->setMaxHeight(View::AUTO); });

    this->registerFloatXMLAttribute("maxHeight", [this](float value)
        { this->setMaxHeight(value); });

    this->registerPercentageXMLAttribute("maxHeight", [this](float percentage)
        { this->setMaxHeightPercentage(percentage); });

    // Grow and shrink
    this->registerFloatXMLAttribute("grow", [this](float value)
        { this->setGrow(value); });

    this->registerFloatXMLAttribute("shrink", [this](float value)
        { this->setShrink(value); });

    // Alignment
    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "alignSelf", AlignSelf, this->setAlignSelf,
        {
            { "auto", AlignSelf::AUTO },
            { "flexStart", AlignSelf::FLEX_START },
            { "center", AlignSelf::CENTER },
            { "flexEnd", AlignSelf::FLEX_END },
            { "stretch", AlignSelf::STRETCH },
            { "baseline", AlignSelf::BASELINE },
            { "spaceBetween", AlignSelf::SPACE_BETWEEN },
            { "spaceAround", AlignSelf::SPACE_AROUND },
        });

    // Margins all
    this->registerFloatXMLAttribute("margin", [this](float value)
        { this->setMargins(value, value, value, value); });

    this->registerAutoXMLAttribute("margin", [this]
        { this->setMargins(View::AUTO, View::AUTO, View::AUTO, View::AUTO); });

    // Margin top
    this->registerFloatXMLAttribute("marginTop", [this](float value)
        { this->setMarginTop(value); });

    this->registerAutoXMLAttribute("marginTop", [this]
        { this->setMarginTop(View::AUTO); });

    // Margin right
    this->registerFloatXMLAttribute("marginRight", [this](float value)
        { this->setMarginRight(value); });

    this->registerAutoXMLAttribute("marginRight", [this]
        { this->setMarginRight(View::AUTO); });

    // Margin bottom
    this->registerFloatXMLAttribute("marginBottom", [this](float value)
        { this->setMarginBottom(value); });

    this->registerAutoXMLAttribute("marginBottom", [this]
        { this->setMarginBottom(View::AUTO); });

    // Margin left
    this->registerFloatXMLAttribute("marginLeft", [this](float value)
        { this->setMarginLeft(value); });

    this->registerAutoXMLAttribute("marginLeft", [this]
        { this->setMarginLeft(View::AUTO); });

    // Line
    this->registerColorXMLAttribute("lineColor", [this](NVGcolor color)
        { this->setLineColor(color); });

    this->registerFloatXMLAttribute("lineTop", [this](float value)
        { this->setLineTop(value); });

    this->registerFloatXMLAttribute("lineRight", [this](float value)
        { this->setLineRight(value); });

    this->registerFloatXMLAttribute("lineBottom", [this](float value)
        { this->setLineBottom(value); });

    this->registerFloatXMLAttribute("lineLeft", [this](float value)
        { this->setLineLeft(value); });

    // Position
    this->registerFloatXMLAttribute("positionTop", [this](float value)
        { this->setPositionTop(value); });

    this->registerFloatXMLAttribute("positionRight", [this](float value)
        { this->setPositionRight(value); });

    this->registerFloatXMLAttribute("positionBottom", [this](float value)
        { this->setPositionBottom(value); });

    this->registerFloatXMLAttribute("positionLeft", [this](float value)
        { this->setPositionLeft(value); });

    this->registerPercentageXMLAttribute("positionTop", [this](float value)
        { this->setPositionTopPercentage(value); });

    this->registerPercentageXMLAttribute("positionRight", [this](float value)
        { this->setPositionRightPercentage(value); });

    this->registerPercentageXMLAttribute("positionBottom", [this](float value)
        { this->setPositionBottomPercentage(value); });

    this->registerPercentageXMLAttribute("positionLeft", [this](float value)
        { this->setPositionLeftPercentage(value); });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "positionType", PositionType, this->setPositionType,
        {
            { "relative", PositionType::RELATIVE },
            { "absolute", PositionType::ABSOLUTE },
            { "static", PositionType::STATIC },
        });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "boxSizing", BoxSizing, this->setBoxSizing,
        {
            { "borderBox", BoxSizing::BORDER_BOX },
            { "contentBox", BoxSizing::CONTENT_BOX },
        });

    // Custom focus routes
    this->registerStringXMLAttribute("focusUp", [this](std::string value)
        { this->setCustomNavigationRoute(FocusDirection::UP, value); });

    this->registerStringXMLAttribute("focusRight", [this](std::string value)
        { this->setCustomNavigationRoute(FocusDirection::RIGHT, value); });

    this->registerStringXMLAttribute("focusDown", [this](std::string value)
        { this->setCustomNavigationRoute(FocusDirection::DOWN, value); });

    this->registerStringXMLAttribute("focusLeft", [this](std::string value)
        { this->setCustomNavigationRoute(FocusDirection::LEFT, value); });

    // Shape
    this->registerColorXMLAttribute("backgroundColor", [this](NVGcolor value)
        { this->setBackgroundColor(value); });

    this->registerColorXMLAttribute("borderColor", [this](NVGcolor value)
        { this->setBorderColor(value); });

    this->registerFloatXMLAttribute("borderThickness", [this](float value)
        { this->setBorderThickness(value); });

    this->registerFloatXMLAttribute("cornerRadius", [this](float value)
        { this->setCornerRadius(value); });

    this->registerFloatXMLAttribute("cornerTopLeftRadius", [this](float value)
        { this->setCornerRadius(value, this->cornerRadii[1], this->cornerRadii[2], this->cornerRadii[3]); });

    this->registerFloatXMLAttribute("cornerTopRightRadius", [this](float value)
        { this->setCornerRadius(this->cornerRadii[0], value, this->cornerRadii[2], this->cornerRadii[3]); });

    this->registerFloatXMLAttribute("cornerBottomRightRadius", [this](float value)
        { this->setCornerRadius(this->cornerRadii[0], this->cornerRadii[1], value, this->cornerRadii[3]); });

    this->registerFloatXMLAttribute("cornerBottomLeftRadius", [this](float value)
        { this->setCornerRadius(this->cornerRadii[0], this->cornerRadii[1], this->cornerRadii[2], value); });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "shadowType", ShadowType, this->setShadowType,
        {
            {
                "none",
                ShadowType::NONE,
            },
            {
                "generic",
                ShadowType::GENERIC,
            },
            {
                "custom",
                ShadowType::CUSTOM,
            },
        });

    // Misc
    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "visibility", Visibility, this->setVisibility,
        {
            { "visible", Visibility::VISIBLE },
            { "invisible", Visibility::INVISIBLE },
            { "gone", Visibility::GONE },
        });

    this->registerStringXMLAttribute("id", [this](std::string value)
        { this->setId(value); });

    AURORE_REGISTER_ENUM_XML_ATTRIBUTE(
        "background", ViewBackground, this->setBackground,
        {
            { "sidebar", ViewBackground::SIDEBAR },
            { "backdrop", ViewBackground::BACKDROP },
            { "vertical_linear", ViewBackground::VERTICAL_LINEAR },
            { "horizontal_linear", ViewBackground::HORIZONTAL_LINEAR },
        });

    // background start and end color for vertical linear style
    this->registerColorXMLAttribute("backgroundStartColor", [this](NVGcolor value)
        { this->backgroundStartColor = value; });
    this->registerColorXMLAttribute("backgroundEndColor", [this](NVGcolor value)
        { this->backgroundEndColor = value; });

    // background corner radius for vertical linear style
    this->registerFloatXMLAttribute("backgroundTopLeftRadius", [this](float value)
        { this->backgroundRadius[0] = value; });
    this->registerFloatXMLAttribute("backgroundTopRightRadius", [this](float value)
        { this->backgroundRadius[1] = value; });
    this->registerFloatXMLAttribute("backgroundBottomRightRadius", [this](float value)
        { this->backgroundRadius[2] = value; });
    this->registerFloatXMLAttribute("backgroundBottomLeftRadius", [this](float value)
        { this->backgroundRadius[3] = value; });

    this->registerBoolXMLAttribute("focusable", [this](bool value)
        { this->setFocusable(value); });

    this->registerBoolXMLAttribute("wireframe", [this](bool value)
        { this->setWireframeEnabled(value); });

    // Highlight
    this->registerBoolXMLAttribute("hideHighlightBackground", [this](bool value)
        { this->setHideHighlightBackground(value); });

    // Highlight
    this->registerBoolXMLAttribute("hideHighlightBorder", [this](bool value)
        { this->setHideHighlightBorder(value); });

    // Highlight
    this->registerBoolXMLAttribute("hideClickAnimation", [this](bool value)
        { this->setHideClickAnimation(value); });

    // Highlight
    this->registerBoolXMLAttribute("hideHighlight", [this](bool value)
        { this->setHideHighlight(value); });

    this->registerFloatXMLAttribute("highlightPadding", [this](float value)
        { this->setHighlightPadding(value); });

    this->registerFloatXMLAttribute("highlightCornerRadius", [this](float value)
        { this->setHighlightCornerRadius(value); });

    // Misc
    this->registerStringXMLAttribute("title", [this](std::string value)
        { this->getAppletFrameItem()->title = value; });

    this->registerFilePathXMLAttribute("icon", [this](std::string value)
        { this->getAppletFrameItem()->setIconFromFile(value); });

    this->registerFloatXMLAttribute("detachedX", [this](float value)
        {
        this->detach();
        this->setDetachedPositionX(value); });

    this->registerFloatXMLAttribute("detachedY", [this](float value)
        {
        this->detach();
        this->setDetachedPositionY(value); });

    this->registerFloatXMLAttribute("alpha", [this](float value)
        { this->setAlpha(value); });

    this->registerFloatXMLAttribute("scale", [this](float value)
        { this->setScale(value); });

    this->registerBoolXMLAttribute("clipsToBounds", [this](float value)
        { this->setClipsToBounds(value); });

    this->registerBoolXMLAttribute("culled", [this](float value)
        { this->setCulled(value); });

    this->registerStringXMLAttribute("transition", [this](std::string value)
        {
        if (!this->setTransition(value))
            Logger::error("Cannot read transition=\"{}\" on {}", value, this->describe()); });

    this->registerStringXMLAttribute("bind", [this](std::string value)
        {
        if (!this->bind(value))
            Logger::error("Cannot read bind=\"{}\" on {}", value, this->describe()); });

    this->registerStringXMLAttribute("accessibilityLabel", [this](std::string value)
        { this->setAccessibilityLabel(value); });

    this->registerStringXMLAttribute("accessibilityValue", [this](std::string value)
        { this->setAccessibilityValue(value); });

    this->registerBoolXMLAttribute("accessibilitySelected", [this](bool value)
        { this->setAccessibilitySelected(value); });

    this->registerStringXMLAttribute("accessibilityRole", [this](std::string value)
        {
        static const std::unordered_map<std::string, AccessibilityRole> ROLES = {
            { "none", AccessibilityRole::NONE },
            { "button", AccessibilityRole::BUTTON },
            { "label", AccessibilityRole::LABEL },
            { "header", AccessibilityRole::HEADER },
            { "image", AccessibilityRole::IMAGE },
            { "cell", AccessibilityRole::CELL },
            { "toggle", AccessibilityRole::TOGGLE },
            { "slider", AccessibilityRole::SLIDER },
            { "text_field", AccessibilityRole::TEXT_FIELD },
            { "tab", AccessibilityRole::TAB },
            { "list", AccessibilityRole::LIST },
            { "dialog", AccessibilityRole::DIALOG },
        };
        auto it = ROLES.find(value);
        if (it == ROLES.end())
            fatal("Illegal value \"" + value + "\" for XML attribute \"accessibilityRole\"");
        this->setAccessibilityRole(it->second); });

    this->registerFloatXMLAttribute("aspectRatio", [this](float value)
        { this->setAspectRatio(value); });
}

void View::setTranslationY(float translationY)
{
    this->translation.y = translationY;
}

void View::setTranslationX(float translationX)
{
    this->translation.x = translationX;
}

void View::setVisibility(Visibility visibility)
{
    // Only change YG properties and invalidate if going from or to GONE
    if ((this->visibility == Visibility::GONE && visibility != Visibility::GONE) || (this->visibility != Visibility::GONE && visibility == Visibility::GONE))
    {
        if (visibility == Visibility::GONE)
            YGNodeStyleSetDisplay(this->ygNode, YGDisplayNone);
        else
            YGNodeStyleSetDisplay(this->ygNode, YGDisplayFlex);

        this->invalidate();
    }

    this->visibility = visibility;

    if (visibility == Visibility::VISIBLE)
        this->willAppear();
    else
        this->willDisappear();
}

Visibility View::getVisibility()
{
    return visibility;
}

void View::printXMLAttributeErrorMessage(tinyxml2::XMLElement* element, std::string name, std::string value)
{
    this->ensureCommonAttributes();
    ReadingXMLElement reading(element);
    if (this->knownAttributes.contains(name))
        fatalXML("Illegal value \"" + value + "\" for XML attribute \"" + name + "\"");
    std::string closest = closestXMLName(name, std::vector<std::string>(this->knownAttributes.begin(), this->knownAttributes.end()));
    fatalXML("Unknown XML attribute \"" + name + "\" (with value \"" + value + "\")" + (closest.empty() ? "" : "; did you mean \"" + closest + "\"?"));
}

void View::registerFloatXMLAttribute(std::string name, FloatAttributeHandler handler)
{
    if (this->registeringCommonAttributes && this->floatAttributes.contains(name))
        return; // the subclass's own, registered first
    this->floatAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerPercentageXMLAttribute(std::string name, FloatAttributeHandler handler)
{
    if (this->registeringCommonAttributes && this->percentageAttributes.contains(name))
        return; // the subclass's own, registered first
    this->percentageAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerAutoXMLAttribute(std::string name, AutoAttributeHandler handler)
{
    if (this->registeringCommonAttributes && this->autoAttributes.contains(name))
        return; // the subclass's own, registered first
    this->autoAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerStringXMLAttribute(std::string name, StringAttributeHandler handler)
{
    if (this->registeringCommonAttributes && this->stringAttributes.contains(name))
        return; // the subclass's own, registered first
    this->stringAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerColorXMLAttribute(std::string name, ColorAttributeHandler handler)
{
    if (this->registeringCommonAttributes && this->colorAttributes.contains(name))
        return; // the subclass's own, registered first
    this->colorAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerBoolXMLAttribute(std::string name, BoolAttributeHandler handler)
{
    if (this->registeringCommonAttributes && this->boolAttributes.contains(name))
        return; // the subclass's own, registered first
    this->boolAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerFilePathXMLAttribute(std::string name, FilePathAttributeHandler handler)
{
    if (this->registeringCommonAttributes && this->filePathAttributes.contains(name))
        return; // the subclass's own, registered first
    this->filePathAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

float ntz(float value)
{
    return std::isnan(value) ? 0.0f : value;
}

View* View::getView(std::string id)
{
    if (id == this->id)
        return this;

    return nullptr;
}

View* View::getNearestView(std::string id)
{
    // First try a children of ours
    View* child = this->getView(id);

    if (child)
        return child;

    // Then go up one level and try again
    if (this->hasParent())
        return this->getParent()->getNearestView(id);

    return nullptr;
}

void View::setId(std::string id)
{
    if (id == "")
        fatal("ID cannot be empty");

    this->id = id;
}

bool View::isFocusable()
{
    return this->focusable && this->visibility == Visibility::VISIBLE;
}

View* View::getDefaultFocus()
{
    if (this->isFocusable())
        return this;

    return nullptr;
}

View* View::hitTest(Point point)
{
    // Check if can focus ourself first
    if (!this->isFocusable() || alpha == 0.0f || visibility != Visibility::VISIBLE)
        return nullptr;

    Rect frame = getFrame();
    //    Logger::debug(describe() + ": --- " + frame.describe());

    if (frame.pointInside(point))
    {
        //        Logger::debug(describe() + ": OK");
        return this;
    }

    return nullptr;
}

// Keyed by the path itself: two paths with the same hash would have
// shared a document. (string_view::data() is not null-terminated either.)
static std::unordered_map<std::string, std::shared_ptr<tinyxml2::XMLDocument>>& xmlCache()
{
    static std::unordered_map<std::string, std::shared_ptr<tinyxml2::XMLDocument>> cache;
    return cache;
}

std::shared_ptr<tinyxml2::XMLDocument> View::getXMLCache(std::string_view path)
{
    auto& doc = xmlCache()[std::string(path)];
    if (!doc)
        doc = std::make_shared<tinyxml2::XMLDocument>();
    return doc;
}

void View::forgetXMLCache(const std::string& path)
{
    static std::vector<std::shared_ptr<tinyxml2::XMLDocument>> retired;
    auto it = xmlCache().find(path);
    if (it == xmlCache().end())
        return;
    retired.push_back(it->second);
    xmlCache().erase(it);
}

std::string View::currentXMLResource;

std::string View::describeXMLError(tinyxml2::XMLDocument* document, const std::string& source)
{
    std::string where = source.empty() ? View::currentXMLResource : source;
    if (where.empty())
        where = "XML string";

    // ErrorStr() names the error, its id, the line and what tinyxml2 found there
    return where + ": " + document->ErrorStr();
}

void View::setWireframeEnabled(bool wireframe)
{
    this->wireframeEnabled = wireframe;
}

bool View::isWireframeEnabled()
{
    return this->wireframeEnabled;
}

AppletFrame* View::getAppletFrame()
{
    View* view = this;
    while (view)
    {
        AppletFrame* applet = dynamic_cast<AppletFrame*>(view);
        if (applet)
            return applet;

        view = view->getParent();
    }
    return nullptr;
}

void View::updateAppletFrameItem()
{
    AppletFrame* appletFrame = this->getAppletFrame();
    if (appletFrame)
    {
        appletFrame->updateAppletFrameItem();
        appletFrame->setTitle(getAppletFrameItem()->title);
        appletFrame->setIcon(getAppletFrameItem()->iconPath);
    }
}

void View::present(View* view)
{
    AppletFrame* applet = getAppletFrame();
    if (!applet)
        return;

    applet->pushContentView(view);
}

void View::dismiss(std::function<void(void)> cb)
{
    AppletFrame* applet = getAppletFrame();
    if (!applet)
        return;

    applet->popContentView(cb);
}

void View::ptrLock()
{
    ptrLockCounter++;
}

void View::ptrUnlock()
{
    ptrLockCounter--;
}

std::weak_ptr<void> View::aliveToken()
{
    if (!alive)
        alive = std::make_shared<char>(0);
    return alive;
}

bool View::isPtrLocked()
{
    return ptrLockCounter > 0;
}

void View::freeView()
{
    Application::addToFreeQueue(this);
}

Activity* View::getParentActivity()
{
    if (parentActivity != nullptr)
        return parentActivity;

    if (parent != nullptr)
        return parent->getParentActivity();

    return nullptr;
}

void View::setParentActivity(Activity* activity)
{
    parentActivity = activity;
}

} // namespace aurore
