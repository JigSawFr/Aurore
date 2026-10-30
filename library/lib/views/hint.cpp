// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/application.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/thread.hpp>
#include <aurore/core/touch/tap_gesture.hpp>
#include <aurore/core/util.hpp>
#include <aurore/views/applet_frame.hpp>
#include <aurore/views/hint.hpp>

using namespace aurore::literals;

namespace aurore
{

constexpr std::string_view hintXML = R"xml(
    <aurore:Box
        width="auto"
        height="auto"
        axis="row"
        paddingTop="4"
        paddingBottom="4"
        paddingLeft="16"
        paddingRight="16"
        cornerRadius="6">
            <aurore:Label
                id="icon"
                width="auto"
                height="auto"
                fontSize="25.5"/>

            <aurore:Label
                id="hint"
                width="auto"
                height="auto"
                fontSize="21.5"
                marginLeft="8"/>

    </aurore:Box>
)xml";

static bool hintsSteady         = false;
static View* hintsOptionalOwner = nullptr;

void setHintsOptionalOwner(View* owner)
{
    hintsOptionalOwner = owner;
}

View* getHintsOptionalOwner()
{
    return hintsOptionalOwner;
}

static float textFullWidth(Label* label)
{
    NVGcontext* vg = Application::getNVGContext();
    nvgFontFaceId(vg, label->getFont());
    nvgFontSize(vg, label->getFullFontSize());
    float bounds[4];
    nvgTextBounds(vg, 0, 0, label->getFullText().c_str(), nullptr, bounds);
    return bounds[2] - bounds[0];
}

float Hint::fullWidth()
{
    // As hintXML lays it out: 16 on each side, 8 between the key and the text.
    return 16 + textFullWidth(icon) + 8 + textFullWidth(hint) + 16;
}

void setHintsSteady(bool steady)
{
    hintsSteady = steady;
}

Hint::Hint(std::shared_ptr<Action> action, bool allowAButtonTouch)
    : Box(Axis::ROW)
    , action(action)
{
    this->inflateFromXMLString(hintXML);
    this->setFocusable(false);
    // The bottom bar has little room to grow: its hints at most this larger.
    icon->setFontScaleMax(getHintsFontScaleMax());
    icon->setShrinkToFit(true);
    hint->setFontScaleMax(getHintsFontScaleMax());
    hint->setShrinkToFit(true);

    icon->setText(getKeyIcon(static_cast<ControllerButton>(action->getButton())));
    hint->setText(action->getHintText());

    const bool blocked = !hintsSteady && Application::isInputBlocks();
    if ((action->getButton() != BUTTON_A || allowAButtonTouch) && action->isAvailable() && !blocked)
    {
        this->addGestureRecognizer(new TapGestureRecognizer(this, [this, action]()
            { action->getActionListener()(this); }));
    }

    if (!action->isAvailable() || blocked)
    {
        icon->setThemeColor("textColor", "aurore/text_disabled");
        hint->setThemeColor("textColor", "aurore/text_disabled");
    }
}

std::string Hint::getKeyIcon(ControllerButton button, bool ignoreKeysSwap)
{
    if (!ignoreKeysSwap)
        button = InputManager::mapControllerState(button);

    switch (button)
    {
        case BUTTON_A:
            return "\uE0E0";
        case BUTTON_B:
            return "\uE0E1";
        case BUTTON_X:
            return "\uE0E2";
        case BUTTON_Y:
            return "\uE0E3";
        case BUTTON_LSB:
            return "\uE104";
        case BUTTON_RSB:
            return "\uE105";
        case BUTTON_LT:
            return "\uE0E6";
        case BUTTON_RT:
            return "\uE0E7";
        case BUTTON_LB:
            return "\uE0E4";
        case BUTTON_RB:
            return "\uE0E5";
        case BUTTON_START:
            return "\uE0EF";
        case BUTTON_BACK:
            return "\uE0F0";
        case BUTTON_LEFT:
            return "\uE0ED";
        case BUTTON_UP:
            return "\uE0EB";
        case BUTTON_RIGHT:
            return "\uE0EE";
        case BUTTON_DOWN:
            return "\uE0EC";
        default:
            return "\uE152";
    }
}

static float hintsFontScaleMax = 1.0f;

void setHintsFontScaleMax(float max)
{
    hintsFontScaleMax = max;
}

float getHintsFontScaleMax()
{
    return hintsFontScaleMax;
}

Hints::Hints()
{
    setAxis(Axis::ROW);
    setDirection(Direction::LEFT_TO_RIGHT);

    hintSubscription = Application::getGlobalHintsUpdateEvent()->subscribe([this]()
        {
        if (!AppletFrame::HIDE_BOTTOM_BAR || forceShown)
        {
            refillHints(Application::getCurrentFocus());
        } });

    this->registerBoolXMLAttribute("addBaseAction", [this](bool value)
        { this->setAddUnableAButtonAction(value); });

    this->registerBoolXMLAttribute("allowAButtonTouch", [this](bool value)
        { this->setAllowAButtonTouch(value); });

    this->registerBoolXMLAttribute("forceShown", [this](bool value)
        { this->forceShown = value; });
}

Hints::~Hints()
{
    Application::getGlobalHintsUpdateEvent()->unsubscribe(hintSubscription);
}

static int buttonToSortableVal(ControllerButton button)
{
    // From left to right:
    //  - first +
    //  - then all hints that are not B and A in original order
    //  - finally B and A

    switch (button)
    {
        case BUTTON_START:
            return 0;
        case BUTTON_B:
            return 2;
        case BUTTON_A:
            return 3;
        default:
            return 1;
    }
}

static bool actionsSortFunc(const std::shared_ptr<Action>& a, const std::shared_ptr<Action>& b)
{
    return buttonToSortableVal(static_cast<ControllerButton>(a->getButton())) < buttonToSortableVal(static_cast<ControllerButton>(b->getButton()));
}

void Hints::refillHints(View* focusView)
{
    if (!focusView)
        return;

    // todo: 做一个缓存，可以节约 Hint 组件生成
    clearViews();

    std::set<ControllerButton> addedButtons; // we only ever want one action per key
    std::vector<std::shared_ptr<Action>> actions;
    std::set<Action*> optional;

    while (focusView != nullptr)
    {
        for (auto& action : focusView->getActions())
        {
            if (action->getType() != ACTION_GAMEPAD)
                continue; // only show gamepad actions in hints

            if (action->isHidden())
                continue;

            if (addedButtons.find(static_cast<ControllerButton>(action->getButton())) != addedButtons.end())
                continue;

            if (Application::isHintsLiteMode() && action->getButton() != BUTTON_A && action->getButton() != BUTTON_B)
            {
                continue;
            }

            addedButtons.insert(static_cast<ControllerButton>(action->getButton()));
            // With nothing to say, no hint, but the button stays this
            // action's (a parent's hint for it would be wrong)
            if (action->getHintText().empty())
                continue;
            actions.push_back(action);
            if (focusView == hintsOptionalOwner && action->getButton() != BUTTON_A && action->getButton() != BUTTON_B)
                optional.insert(action.get());
        }

        focusView = focusView->getParent();
    }

    const auto it = std::find_if(actions.begin(), actions.end(), [](const std::shared_ptr<Action>& action)
        { return *action == BUTTON_A; });

    if (addUnableAButtonAction && it == actions.end())
    {
        actions.push_back(std::make_shared<GamepadAction>(BUTTON_A, 0, "hints/ok"_i18n, false, false, false, Sound::SOUND_NONE, nullptr));
    }

    // Sort the actions
    std::stable_sort(actions.begin(), actions.end(), actionsSortFunc);

    std::vector<Hint*> hints;
    float need = 0;
    for (auto action : actions)
    {
        hints.push_back(new Hint(action, allowAButtonTouch));
        need += hints.back()->fullWidth();
    }

    // Without room for them all, the optional hints out, the last one first
    // (before the footer is laid out, once it is: onLayout).
    const float available = room();
    roomUnknown           = available <= 0;
    for (size_t i = actions.size(); i-- > 0 && !roomUnknown && need > available;)
    {
        if (!optional.contains(actions[i].get()))
            continue;
        need -= hints[i]->fullWidth();
        delete hints[i];
        hints[i] = nullptr;
    }

    for (Hint* hint : hints)
        if (hint)
            addView(hint);
}

// The width the hints have in the footer: its row but the clock, and a gap.
float Hints::room()
{
    Box* row = getParent();
    if (!row || row->getWidth() <= 0)
        return 0;
    float width = row->getWidth() - 16;
    for (View* view : row->getChildren())
        if (view != this)
            width -= view->getWidth();
    return width;
}

void Hints::onLayout()
{
    // Filled before the footer had its size: again, once laid out.
    if (!roomUnknown || !hintsOptionalOwner || room() <= 0)
        return;
    roomUnknown = false;
    aurore::sync([]()
        { Application::getGlobalHintsUpdateEvent()->fire(); });
}

View* Hints::create()
{
    return new Hints();
}

} // namespace aurore