// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include <aurore/core/application.hpp>
#include <aurore/core/touch/pan_gesture.hpp>
#include <aurore/core/util.hpp>
#include <aurore/views/cells/cell_detail.hpp>
#include <aurore/views/slider.hpp>

namespace aurore
{

// The knob, which takes the focus: it says what its slider is set to.
class SliderPointer : public Rectangle
{
  public:
    explicit SliderPointer(Slider* slider)
        : slider(slider)
    {
    }

  protected:
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::SLIDER; }
    std::string getDefaultAccessibilityLabel() override { return this->slider->getAccessibilityLabel(); }
    std::string getDefaultAccessibilityValue() override { return this->slider->getAccessibilityValue(); }

  private:
    Slider* slider;
};

Slider::Slider()
{
    line      = new Rectangle();
    lineEmpty = new Rectangle();
    pointer   = new SliderPointer(this);

    line->detach();
    lineEmpty->detach();
    pointer->detach();

    addView(pointer);
    addView(line);
    addView(lineEmpty);

    setHeight(40);

    line->setHeight(7);
    line->setCornerRadius(3.5f);

    lineEmpty->setHeight(7);
    lineEmpty->setCornerRadius(3.5f);

    pointer->setHeight(38);
    pointer->setWidth(38);
    pointer->setCornerRadius(19);
    pointer->setHighlightCornerRadius(21);
    pointer->setBorderThickness(4);
    pointer->setShadowType(ShadowType::GENERIC);
    pointer->setShadowVisibility(true);
    pointer->setFocusable(true);

    pointer->setThemeColor("color", "aurore/slider/pointer_color");
    pointer->setThemeColor("borderColor", "aurore/slider/pointer_border_color");

    line->setThemeColor("color", "aurore/slider/line_filled");
    lineEmpty->setThemeColor("color", "aurore/slider/line_empty");

    // The D-pad moves the slider (in buttonsProcessing, or here a step at a
    // time with a discrete step) rather than the focus
    pointer->registerAction(
        "Right Click Blocker", BUTTON_NAV_RIGHT, [this](View* view)
        {
            if (this->discreteStep > 0)
                this->moveByStep(1);
            return true; },
        true, true, SOUND_NONE);

    pointer->registerAction(
        "Right Click Blocker", BUTTON_NAV_LEFT, [this](View* view)
        {
            if (this->discreteStep > 0)
                this->moveByStep(-1);
            return true; },
        true, true, SOUND_NONE);

    pointer->registerAction(
        "A Button Click Blocker", BUTTON_A, [](View* view)
        { return true; },
        true, false, SOUND_NONE);

    pointer->addGestureRecognizer(new PanGestureRecognizer([this](PanGestureStatus status, Sound* soundToPlay)
        {
        Application::giveFocus(pointer);

        static float lastProgress = progress;

        if (status.state == GestureState::UNSURE)
        {
            *soundToPlay = SOUND_FOCUS_CHANGE;
            return;
        }

        else if (status.state == GestureState::INTERRUPTED || status.state == GestureState::FAILED)
        {
            *soundToPlay = SOUND_TOUCH_UNFOCUS;
            return;
        }

        else if (status.state == GestureState::START)
        {
            lastProgress = progress;
            progressing  = true;
        }

        float paddingWidth = getWidth() - pointer->getWidth();
        float delta        = status.position.x - status.startPosition.x;

        setProgress(lastProgress + delta / paddingWidth);

        if (status.state == GestureState::END)
        {
            Application::getPlatform()->getAudioPlayer()->play(SOUND_SLIDER_RELEASE);
            progressing = false;
            inputProgressEvent.fire(progress);
            announceValue();
        } },
        PanAxis::HORIZONTAL));

    progress = 0.33f;
}

void Slider::onLayout()
{
    Box::onLayout();
    updateUI();
}

View* Slider::getDefaultFocus()
{
    if (this->getVisibility() != Visibility::VISIBLE)
        return nullptr;

    return pointer;
}

void Slider::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    buttonsProcessing();
    Box::draw(vg, x, y, width, height, style, ctx);
}

void Slider::buttonsProcessing()
{
    if (pointer->isFocused())
    {
        auto& state        = Application::getControllerState();
        static bool repeat = false;

        bool held = state.buttons[BUTTON_NAV_RIGHT] || state.buttons[BUTTON_NAV_LEFT];
        if (held)
            progressing = true;
        else if (progressing)
        {
            // Let go: the value is chosen
            progressing = false;
            inputProgressEvent.fire(progress);
            announceValue();
        }

        // A discrete step moves on each press, in the actions
        if (this->discreteStep > 0)
            return;

        if (state.buttons[BUTTON_NAV_RIGHT] && state.buttons[BUTTON_NAV_LEFT])
            return;

        if (state.buttons[BUTTON_NAV_RIGHT])
        {
            setProgress(progress += step / Application::getFPS());
            if (progress >= 1 && !repeat)
            {
                repeat = true;
                pointer->shakeHighlight(FocusDirection::RIGHT);
                Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
            }
        }

        if (state.buttons[BUTTON_NAV_LEFT])
        {
            setProgress(progress -= step / Application::getFPS());
            if (progress <= 0 && !repeat)
            {
                repeat = true;
                pointer->shakeHighlight(FocusDirection::LEFT);
                Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
            }
        }

        if ((!state.buttons[BUTTON_NAV_RIGHT] && !state.buttons[BUTTON_NAV_LEFT]) || (progress > 0.01f && progress < 0.99f))
        {
            repeat = false;
        }
    }
}

void Slider::setProgress(float progress)
{
    static int lastProgressTicker = this->progress * 10;

    this->progress = progress;

    if (this->progress < 0)
        this->progress = 0;

    if (this->progress > 1)
        this->progress = 1;

    if (lastProgressTicker != (int)(this->progress * 10))
    {
        lastProgressTicker = this->progress * 10;
        Application::getAudioPlayer()->play(SOUND_SLIDER_TICK);
    }

    progressEvent.fire(this->progress);
    updateUI();

    // While held or dragged, the value is said once let go.
    if (!this->progressing || this->discreteStep > 0)
        this->announceValue();
}

void Slider::announceValue()
{
    int percent = (int)std::lround(this->progress * 100);
    if (percent == this->announced || Application::getCurrentFocus() != pointer)
        return;
    this->announced = percent;
    pointer->announceAccessibilityValue();
}

std::string Slider::getDefaultAccessibilityLabel()
{
    // A slider cell's title
    View* up = this->hasParent() ? this->getParent() : nullptr;
    for (int depth = 0; up && depth < 3; depth++, up = up->hasParent() ? up->getParent() : nullptr)
        if (auto* cell = dynamic_cast<DetailCell*>(up))
            return cell->title->getFullText();
    return "";
}

std::string Slider::getDefaultAccessibilityValue()
{
    return accessibilityPercent(this->progress);
}

void Slider::updateUI()
{
    float paddingWidth   = getWidth() - pointer->getWidth();
    float lineStart      = pointer->getWidth() / 2;
    float lineStartWidth = paddingWidth * progress;
    float lineEnd        = paddingWidth * progress + pointer->getWidth() / 2;
    float lineEndWidth   = paddingWidth * (1 - progress);
    float lineYPos       = getHeight() / 2 - line->getHeight() / 2;

    line->setDetachedPosition(lineStart, lineYPos);
    line->setWidth(lineStartWidth);

    lineEmpty->setDetachedPosition(round(lineEnd), lineYPos);
    lineEmpty->setWidth(lineEndWidth);

    pointer->setDetachedPosition(lineEnd - pointer->getWidth() / 2, getHeight() / 2 - pointer->getHeight() / 2);
}

float Slider::getProgress()
{
    return progress;
}

Event<float>* Slider::getProgressEvent()
{
    return &progressEvent;
}

Event<float>* Slider::getInputProgressEvent()
{
    return &inputProgressEvent;
}

bool Slider::isProgressing()
{
    return this->progressing;
}

void Slider::setDiscreteStep(float step)
{
    this->discreteStep = step;
}

void Slider::moveByStep(int direction)
{
    // On the grid of the step, so a value set from elsewhere snaps to it
    float steps = std::round(this->progress / this->discreteStep) + direction;
    float next  = steps * this->discreteStep;
    if ((direction > 0 && this->progress >= 1) || (direction < 0 && this->progress <= 0))
    {
        pointer->shakeHighlight(direction > 0 ? FocusDirection::RIGHT : FocusDirection::LEFT);
        Application::getAudioPlayer()->play(SOUND_FOCUS_ERROR);
        return;
    }
    this->setProgress(next);
}

void Slider::setStep(float step)
{
    this->step = step;
}

void Slider::setPointerSize(float size)
{
    this->pointer->setWidth(size);
    this->pointer->setHeight(size);
    this->updateUI();
}

View* Slider::create()
{
    return new Slider();
}

} // namespace aurore
