/*
    Copyright 2021 XITRIX

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

#pragma once

#include <aurore/core/application.hpp>
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/image.hpp>
#include <aurore/views/label.hpp>

namespace aurore
{

// The footer's hints and clock with a larger text (Label::setFontScale):
// drawn at most `max` times their size (1 by default: they keep theirs),
// smaller again when the footer has no room for them.
void setHintsFontScaleMax(float max);
float getHintsFontScaleMax();

class Hint : public Box
{
  public:
    Hint(std::shared_ptr<Action> action, bool allowAButtonTouch = false);
    static std::string getKeyIcon(ControllerButton button, bool ignoreKeysSwap = false);

    // Its width with its key and text at their full size.
    float fullWidth();

  private:
    std::shared_ptr<Action> action;

    BRLS_BIND(Label, icon, "icon");
    BRLS_BIND(Label, hint, "hint");
};

class Hints : public Box
{
  public:
    Hints();
    ~Hints();

    void setAddUnableAButtonAction(bool value)
    {
        addUnableAButtonAction = value;
    }

    bool getAddUnableAButtonAction() const
    {
        return addUnableAButtonAction;
    }

    void setAllowAButtonTouch(bool value)
    {
        allowAButtonTouch = value;
    }

    bool getAllowAButtonTouch() const
    {
        return allowAButtonTouch;
    }

    static View* create();

    void onLayout() override;

  private:
    void refillHints(View* focusView);
    float room();
    bool roomUnknown            = false;
    bool addUnableAButtonAction = true;
    bool allowAButtonTouch      = false;
    bool forceShown             = false;

    VoidEvent::Subscription hintSubscription;
};

// Draws the hints as they are once a screen has opened or closed: greyed
// only for an action that is not available, not while the input is blocked
// by the screen's fade. So a screenshot does not depend on when it was taken.
void setHintsSteady(bool steady);

// The actions of `owner` (but A and B) are the footer's optional hints: keys
// that work from any screen, which a screen with its own keys may need the
// room of. When the hints do not all fit at their full size, the optional
// ones are left out, the last one first. nullptr (the default): none.
void setHintsOptionalOwner(View* owner);
View* getHintsOptionalOwner();

} // namespace aurore
