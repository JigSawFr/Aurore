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

// The parts of a View that most views never use: state styles, transitions,
// bindings to observables and accessibility. They live in View::Extras,
// allocated the first time one of them is asked for.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <aurore/core/activity.hpp>
#include <aurore/core/application.hpp>
#include <aurore/core/box.hpp>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/observable.hpp>
#include <aurore/core/util.hpp>
#include <aurore/core/view.hpp>
#include <aurore/views/recycler.hpp>

namespace aurore
{

bool StyleValue::operator==(const StyleValue& other) const
{
    if (this->type != other.type)
        return false;
    switch (this->type)
    {
        case Type::NUMBER:
            return this->number == other.number;
        case Type::COLOR:
            return this->color.r == other.color.r && this->color.g == other.color.g
                && this->color.b == other.color.b && this->color.a == other.color.a;
        default:
            return true;
    }
}

StyleValue StyleValue::mix(const StyleValue& from, const StyleValue& to, float t)
{
    if (from.type != to.type)
        return t < 1.0f ? from : to;
    if (from.type == Type::NUMBER)
        return StyleValue::of(from.number + (to.number - from.number) * t);
    if (from.type == Type::COLOR)
    {
        // An overshooting easing (backOut) must not leave the colour range.
        auto channel = [t](float a, float b)
        { return std::clamp(a + (b - a) * t, 0.0f, 1.0f); };
        return StyleValue::of(nvgRGBAf(channel(from.color.r, to.color.r), channel(from.color.g, to.color.g),
            channel(from.color.b, to.color.b), channel(from.color.a, to.color.a)));
    }
    return to;
}

static const std::pair<AccessibilityRole, const char*> ROLE_NAMES[] = {
    { AccessibilityRole::NONE, "none" },
    { AccessibilityRole::BUTTON, "button" },
    { AccessibilityRole::LABEL, "label" },
    { AccessibilityRole::HEADER, "header" },
    { AccessibilityRole::IMAGE, "image" },
    { AccessibilityRole::CELL, "cell" },
    { AccessibilityRole::TOGGLE, "toggle" },
    { AccessibilityRole::SLIDER, "slider" },
    { AccessibilityRole::TEXT_FIELD, "text_field" },
    { AccessibilityRole::TAB, "tab" },
    { AccessibilityRole::LIST, "list" },
    { AccessibilityRole::DIALOG, "dialog" },
};

std::string accessibilityRoleName(AccessibilityRole role)
{
    if (role == AccessibilityRole::NONE)
        return "";
    for (auto& [value, name] : ROLE_NAMES)
        if (value == role)
            return name;
    return "";
}

struct View::Extras
{
    // State styles
    uint8_t states   = 0;
    bool stylesDirty = false;

    struct StateStyle
    {
        StyleState state;
        std::string property;
        std::string value;
    };
    std::vector<StateStyle> stateStyles;

    // Per property a state styles: what it was before a state took it over
    // (base), and what the states set last (applied), to tell the app's own
    // changes from ours.
    struct Styled
    {
        std::string property;
        StyleValue base;
        StyleValue applied;
        bool driven = false;
    };
    std::vector<Styled> styled;

    // Transitions
    struct TransitionSpec
    {
        std::string property; // or "all"
        int32_t duration;
        EasingFunction easing;
    };
    std::vector<TransitionSpec> transitions;

    struct Running
    {
        std::string property;
        StyleValue from;
        StyleValue to;
        Animatable progress;
    };
    std::vector<std::unique_ptr<Running>> running;
    bool applyingTransition = false;

    // The pressed state of a view whose click animation is hidden (nothing
    // plays it back): released after the time the animation would take.
    Animatable pressRelease;

    // Bindings
    std::vector<Subscription> subscriptions;
    std::vector<std::pair<std::string, std::string>> pendingBindings;

    // Accessibility
    bool roleSet           = false;
    AccessibilityRole role = AccessibilityRole::NONE;
    bool labelSet          = false;
    std::string label;
    bool valueSet = false;
    std::string value;
    bool selectedSet = false;
    bool selected    = false;

    ~Extras()
    {
        // The end callbacks do nothing for an animation stopped early, but
        // stop them all before the members they read go.
        for (auto& run : this->running)
            run->progress.stop();
        this->pressRelease.stop();
    }

    const TransitionSpec* findTransition(const std::string& property) const
    {
        const TransitionSpec* all = nullptr;
        for (auto& spec : this->transitions)
        {
            if (spec.property == property)
                return &spec;
            if (spec.property == "all")
                all = &spec;
        }
        return all;
    }

    Running* findRunning(const std::string& property)
    {
        for (auto& run : this->running)
            if (run->property == property)
                return run.get();
        return nullptr;
    }
};

unsigned View::pendingBindings = 0;

void View::ExtrasDeleter::operator()(Extras* extras) const
{
    if (extras)
        View::pendingBindings -= std::min<unsigned>(View::pendingBindings, extras->pendingBindings.size());
    delete extras;
}

View::Extras& View::getExtras()
{
    if (!this->extras)
    {
        this->extras.reset(new Extras());
        if (this->focused)
            this->extras->states |= (uint8_t)StyleState::FOCUS;
    }
    return *this->extras;
}

// ---------------------------------------------------------------------------
// Style properties
// ---------------------------------------------------------------------------

bool View::getStyleProperty(const std::string& name, StyleValue* value)
{
    if (name == "backgroundColor")
        *value = StyleValue::of(this->backgroundColor);
    else if (name == "borderColor")
        *value = StyleValue::of(this->borderColor);
    else if (name == "lineColor")
        *value = StyleValue::of(this->lineColor);
    else if (name == "borderThickness")
        *value = StyleValue::of(this->borderThickness);
    else if (name == "cornerRadius")
        *value = StyleValue::of(this->cornerRadius);
    else if (name == "alpha")
        *value = StyleValue::of(this->alpha.getValue());
    else if (name == "scale")
        *value = StyleValue::of(this->scaleX);
    else
        return false;
    return true;
}

bool View::setStyleProperty(const std::string& name, const StyleValue& value)
{
    bool color  = value.type == StyleValue::Type::COLOR;
    bool number = value.type == StyleValue::Type::NUMBER;
    if (name == "backgroundColor" && color)
        this->setBackgroundColor(value.color);
    else if (name == "borderColor" && color)
        this->setBorderColor(value.color);
    else if (name == "lineColor" && color)
        this->setLineColor(value.color);
    else if (name == "borderThickness" && number)
        this->setBorderThickness(value.number);
    else if (name == "cornerRadius" && number)
        this->setCornerRadius(value.number);
    else if (name == "alpha" && number)
        this->setAlpha(value.number);
    else if (name == "scale" && number)
        this->setScale(value.number);
    else
        return false;
    return true;
}

bool View::parseStyleValue(const std::string& text, StyleValue::Type type, StyleValue* value)
{
    try
    {
        if (type == StyleValue::Type::COLOR)
        {
            NVGcolor color;
            if (startsWith(text, "@theme/"))
                color = Application::getTheme()[text.substr(7)];
            else if (!Theme::parseColor(text, &color))
                return false;
            *value = StyleValue::of(color);
            return true;
        }
        if (type == StyleValue::Type::NUMBER)
        {
            float number;
            if (startsWith(text, "@style/"))
                number = Application::getStyle()[text.substr(7)];
            else
            {
                std::string digits = endsWith(text, "px") ? text.substr(0, text.size() - 2) : text;
                size_t used        = 0;
                number             = std::stof(digits, &used);
                if (used != digits.size() || !std::isfinite(number))
                    return false;
            }
            *value = StyleValue::of(number);
            return true;
        }
    }
    catch (const std::exception& e)
    {
        return false;
    }
    return false;
}

// ---------------------------------------------------------------------------
// State styles
// ---------------------------------------------------------------------------

static const StyleState STATE_PRIORITY[] = { StyleState::DISABLED, StyleState::PRESSED, StyleState::FOCUS, StyleState::HOVER };

bool View::setStateStyle(StyleState state, const std::string& property, const std::string& value)
{
    StyleValue current, parsed;
    if (!this->getStyleProperty(property, &current))
        return false;
    // Checked now for a mistake to show up where it is written; "@theme/"
    // is read again each time the state comes on.
    if (!View::parseStyleValue(value, current.type, &parsed))
        return false;

    Extras& extras = this->getExtras();
    for (auto& style : extras.stateStyles)
    {
        if (style.state == state && style.property == property)
        {
            style.value        = value;
            extras.stylesDirty = true;
            return true;
        }
    }
    extras.stateStyles.push_back({ state, property, value });
    bool known = false;
    for (auto& styled : extras.styled)
        known = known || styled.property == property;
    if (!known)
        extras.styled.push_back({ property });
    extras.stylesDirty = true;
    Application::requestRedraw();
    return true;
}

void View::setStyleState(StyleState state, bool on)
{
    if (!on && !this->extras)
        return;
    Extras& extras = this->getExtras();
    uint8_t states = on ? (extras.states | (uint8_t)state) : (extras.states & ~(uint8_t)state);
    if (states == extras.states)
        return;
    extras.states      = states;
    extras.stylesDirty = true;
    Application::requestRedraw();
}

bool View::hasStyleState(StyleState state)
{
    if (state == StyleState::FOCUS)
        return this->focused;
    return this->extras && (this->extras->states & (uint8_t)state);
}

bool View::hasStateStyle(StyleState state)
{
    if (!this->extras)
        return false;
    for (auto& style : this->extras->stateStyles)
        if (style.state == state)
            return true;
    return false;
}

void View::noteStyleState(StyleState state, bool on)
{
    if (this->extras)
        this->setStyleState(state, on);
}

void View::notePress(bool pressed, bool releaseLater)
{
    if (!this->hasStateStyle(StyleState::PRESSED))
        return;

    this->setStyleState(StyleState::PRESSED, pressed);
    if (!pressed || !releaseLater)
        return;

    Animatable& release = this->extras->pressRelease;
    release.reset(0.0f);
    release.addStep(1.0f, Application::getStyle()["brls/animations/highlight"]);
    release.setEndCallback([this](bool finished)
        {
        if (finished)
            this->setStyleState(StyleState::PRESSED, false); });
    release.start();
}

void View::applyPendingStyles()
{
    if (this->extras && this->extras->stylesDirty)
    {
        this->extras->stylesDirty = false;
        this->updateStateStyles();
    }
}

void View::updateStateStyles()
{
    Extras& extras = *this->extras;
    uint8_t states = extras.states;
    if (this->focused)
        states |= (uint8_t)StyleState::FOCUS;
    else
        states &= ~(uint8_t)StyleState::FOCUS;

    for (auto& styled : extras.styled)
    {
        const std::string* value = nullptr;
        for (StyleState state : STATE_PRIORITY)
        {
            if (!(states & (uint8_t)state))
                continue;
            for (auto& style : extras.stateStyles)
                if (style.state == state && style.property == styled.property)
                    value = &style.value;
            if (value)
                break;
        }

        StyleValue current;
        if (!this->getStyleProperty(styled.property, &current))
            continue;
        // Mid-transition, the property is on its way to the transition's end.
        if (Extras::Running* run = extras.findRunning(styled.property))
            if (run->progress.isRunning())
                current = run->to;

        // Changed by the app since a state set it: that is the new base.
        if (styled.driven && current != styled.applied)
            styled.base = current;

        if (value)
        {
            StyleValue target;
            if (!View::parseStyleValue(*value, current.type, &target))
                continue;
            if (!styled.driven)
            {
                styled.base   = current;
                styled.driven = true;
            }
            styled.applied = target;
            if (current != target)
                this->setStyleProperty(styled.property, target);
        }
        else if (styled.driven)
        {
            styled.driven = false;
            if (current != styled.base)
                this->setStyleProperty(styled.property, styled.base);
        }
    }
}

// ---------------------------------------------------------------------------
// Transitions
// ---------------------------------------------------------------------------

static bool parseEasing(const std::string& name, EasingFunction* easing)
{
    static const std::unordered_map<std::string, EasingFunction> EASINGS = {
        { "linear", EasingFunction::linear },
        { "ease", EasingFunction::cubicInOut },
        { "ease-in", EasingFunction::cubicIn },
        { "ease-out", EasingFunction::cubicOut },
        { "ease-in-out", EasingFunction::cubicInOut },
        { "stepped", EasingFunction::stepped },
        { "quadraticIn", EasingFunction::quadraticIn },
        { "quadraticOut", EasingFunction::quadraticOut },
        { "quadraticInOut", EasingFunction::quadraticInOut },
        { "cubicIn", EasingFunction::cubicIn },
        { "cubicOut", EasingFunction::cubicOut },
        { "cubicInOut", EasingFunction::cubicInOut },
        { "quarticIn", EasingFunction::quarticIn },
        { "quarticOut", EasingFunction::quarticOut },
        { "quarticInOut", EasingFunction::quarticInOut },
        { "quinticIn", EasingFunction::quinticIn },
        { "quinticOut", EasingFunction::quinticOut },
        { "quinticInOut", EasingFunction::quinticInOut },
        { "sinusoidalIn", EasingFunction::sinusoidalIn },
        { "sinusoidalOut", EasingFunction::sinusoidalOut },
        { "sinusoidalInOut", EasingFunction::sinusoidalInOut },
        { "exponentialIn", EasingFunction::exponentialIn },
        { "exponentialOut", EasingFunction::exponentialOut },
        { "exponentialInOut", EasingFunction::exponentialInOut },
        { "circularIn", EasingFunction::circularIn },
        { "circularOut", EasingFunction::circularOut },
        { "circularInOut", EasingFunction::circularInOut },
        { "bounceIn", EasingFunction::bounceIn },
        { "bounceOut", EasingFunction::bounceOut },
        { "bounceInOut", EasingFunction::bounceInOut },
        { "elasticIn", EasingFunction::elasticIn },
        { "elasticOut", EasingFunction::elasticOut },
        { "elasticInOut", EasingFunction::elasticInOut },
        { "backIn", EasingFunction::backIn },
        { "backOut", EasingFunction::backOut },
        { "backInOut", EasingFunction::backInOut },
    };
    auto it = EASINGS.find(name);
    if (it == EASINGS.end())
        return false;
    *easing = it->second;
    return true;
}

static constexpr double MAX_DURATION_MS = 60000;

static bool parseDuration(const std::string& text, int32_t* duration)
{
    try
    {
        size_t used      = 0;
        float value      = std::stof(text, &used);
        std::string unit = text.substr(used);
        double ms;
        if (unit == "ms")
            ms = value;
        else if (unit == "s")
            ms = value * 1000.0;
        else
            return false;
        // Not a number, negative or past a minute: refused, before it is
        // rounded to an int32_t it may not fit
        if (!std::isfinite(ms) || ms < 0 || ms > MAX_DURATION_MS)
            return false;
        *duration = (int32_t)std::lround(ms);
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

static std::vector<std::string> splitList(const std::string& text, const char* separators)
{
    std::vector<std::string> parts;
    std::string current;
    auto flush = [&]
    {
        size_t start = current.find_first_not_of(" \t\n\r");
        size_t end   = current.find_last_not_of(" \t\n\r");
        if (start != std::string::npos)
            parts.push_back(current.substr(start, end - start + 1));
        current.clear();
    };
    for (char c : text)
    {
        if (std::strchr(separators, c))
            flush();
        else
            current += c;
    }
    flush();
    return parts;
}

bool View::setTransition(const std::string& spec)
{
    std::vector<Extras::TransitionSpec> transitions;
    for (const std::string& entry : splitList(spec, ","))
    {
        std::vector<std::string> words = splitList(entry, " \t");
        if (words.size() < 2 || words.size() > 3)
            return false;
        Extras::TransitionSpec transition { words[0], 0, EasingFunction::cubicInOut };
        StyleValue probe;
        if (transition.property != "all" && !this->getStyleProperty(transition.property, &probe))
            return false;
        if (!parseDuration(words[1], &transition.duration))
            return false;
        if (words.size() == 3 && !parseEasing(words[2], &transition.easing))
            return false;
        transitions.push_back(transition);
    }

    if (transitions.empty() && !this->extras)
        return true;
    Extras& extras     = this->getExtras();
    extras.transitions = std::move(transitions);
    return true;
}

bool View::startTransition(const char* name, const StyleValue& to)
{
    Extras& extras = *this->extras;
    if (extras.applyingTransition)
        return false;
    const Extras::TransitionSpec* spec = extras.findTransition(name);
    Extras::Running* run               = extras.findRunning(name);
    if (!spec || spec->duration <= 0 || Application::getReduceMotion())
    {
        // Set at once: a transition still running would undo it.
        if (run)
            run->progress.stop();
        return false;
    }

    if (run && run->progress.isRunning() && run->to == to)
        return true;

    StyleValue from;
    if (!this->getStyleProperty(name, &from) || from.type != to.type)
        return false;
    if (from == to)
    {
        if (run)
            run->progress.stop();
        return false;
    }

    if (!run)
    {
        extras.running.push_back(std::make_unique<Extras::Running>());
        run           = extras.running.back().get();
        run->property = name;
    }
    run->from = from;
    run->to   = to;

    auto apply = [this, run](const StyleValue& value)
    {
        this->extras->applyingTransition = true;
        this->setStyleProperty(run->property, value);
        this->extras->applyingTransition = false;
    };

    run->progress.reset(0.0f);
    run->progress.addStep(1.0f, spec->duration, spec->easing);
    run->progress.setTickCallback([run, apply]
        { apply(StyleValue::mix(run->from, run->to, run->progress.getValue())); });
    run->progress.setEndCallback([run, apply](bool finished)
        {
        if (finished)
            apply(run->to); });
    run->progress.start();
    return true;
}

// ---------------------------------------------------------------------------
// Bindings
// ---------------------------------------------------------------------------

void View::keepSubscription(Subscription&& subscription)
{
    this->getExtras().subscriptions.push_back(std::move(subscription));
}

static std::string formatNumber(float value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%g", value);
    return buffer;
}

bool View::bind(const std::string& spec)
{
    std::vector<std::pair<std::string, std::string>> bindings;
    for (const std::string& entry : splitList(spec, ",;"))
    {
        size_t colon = entry.find(':');
        if (colon == std::string::npos)
            return false;
        std::vector<std::string> property = splitList(entry.substr(0, colon), " \t");
        std::vector<std::string> path     = splitList(entry.substr(colon + 1), " \t");
        if (property.size() != 1 || path.size() != 1 || path[0].find('.') == std::string::npos)
            return false;
        bindings.emplace_back(property[0], path[0]);
    }

    // The activity's models come first: wait for it when it is not known yet.
    Activity* activity = this->getParentActivity();
    for (auto& [property, path] : bindings)
    {
        if (!activity || !this->bindOne(property, path, activity))
        {
            this->getExtras().pendingBindings.emplace_back(property, path);
            View::pendingBindings++;
        }
    }
    return true;
}

bool View::bindOne(const std::string& property, const std::string& path, Activity* activity)
{
    size_t dot             = path.find('.');
    std::string modelName  = path.substr(0, dot);
    std::string key        = path.substr(dot + 1);
    const DataModel* model = activity ? activity->getModel(modelName) : nullptr;
    if (!model)
        model = Application::getModel(modelName);
    if (!model)
        return false;

    if (!model->has(key))
    {
        Logger::error("bind: {} has no \"{}\" ({})", modelName, key, this->describe());
        return true;
    }

    if (property == "text")
    {
        if (auto* observable = model->get<std::string>(key))
        {
            if (!this->setBoundText(observable->get()))
            {
                Logger::error("bind: {} has no text", this->describe());
                return true;
            }
            this->keepSubscription(observable->subscribe([this](const std::string& value)
                { this->setBoundText(value); }));
            return true;
        }
        if (auto* observable = model->get<int>(key))
        {
            this->setBoundText(std::to_string(observable->get()));
            this->keepSubscription(observable->subscribe([this](const int& value)
                { this->setBoundText(std::to_string(value)); }));
            return true;
        }
        if (auto* observable = model->get<float>(key))
        {
            this->setBoundText(formatNumber(observable->get()));
            this->keepSubscription(observable->subscribe([this](const float& value)
                { this->setBoundText(formatNumber(value)); }));
            return true;
        }
    }
    else if (property == "visible")
    {
        if (auto* observable = model->get<bool>(key))
        {
            auto apply = [this](const bool& visible)
            { this->setVisibility(visible ? Visibility::VISIBLE : Visibility::GONE); };
            apply(observable->get());
            this->keepSubscription(observable->subscribe(apply));
            return true;
        }
    }
    else
    {
        StyleValue current;
        if (this->getStyleProperty(property, &current))
        {
            if (current.type == StyleValue::Type::COLOR)
            {
                if (auto* observable = model->get<NVGcolor>(key))
                {
                    auto apply = [this, property](const NVGcolor& color)
                    { this->setStyleProperty(property, StyleValue::of(color)); };
                    apply(observable->get());
                    this->keepSubscription(observable->subscribe(apply));
                    return true;
                }
            }
            else if (auto* observable = model->get<float>(key))
            {
                auto apply = [this, property](const float& number)
                { this->setStyleProperty(property, StyleValue::of(number)); };
                apply(observable->get());
                this->keepSubscription(observable->subscribe(apply));
                return true;
            }
        }
    }

    Logger::error("bind: cannot bind {} of {} to {}", property, this->describe(), path);
    return true;
}

void View::resolveBindings(Activity* activity)
{
    if (View::pendingBindings == 0)
        return;
    if (!activity)
        activity = this->getParentActivity();
    if (!activity)
        return;

    if (this->extras && !this->extras->pendingBindings.empty())
    {
        auto pending = std::move(this->extras->pendingBindings);
        this->extras->pendingBindings.clear();
        View::pendingBindings -= std::min<unsigned>(View::pendingBindings, pending.size());
        for (auto& [property, path] : pending)
        {
            if (!this->bindOne(property, path, activity))
            {
                Logger::warning("bind: no data model for {} ({})", path, this->describe());
                this->extras->pendingBindings.emplace_back(property, path);
                View::pendingBindings++;
            }
        }
    }

    if (Box* box = dynamic_cast<Box*>(this))
        for (View* child : box->getChildren())
            child->resolveBindings(activity);
}

// ---------------------------------------------------------------------------
// Accessibility
// ---------------------------------------------------------------------------

void View::setAccessibilityRole(AccessibilityRole role)
{
    Extras& extras = this->getExtras();
    extras.roleSet = true;
    extras.role    = role;
}

AccessibilityRole View::getAccessibilityRole()
{
    if (this->extras && this->extras->roleSet)
        return this->extras->role;
    return this->getDefaultAccessibilityRole();
}

void View::setAccessibilityLabel(const std::string& label)
{
    Extras& extras  = this->getExtras();
    extras.labelSet = true;
    extras.label    = label;
}

std::string View::getAccessibilityLabel()
{
    if (this->extras && this->extras->labelSet)
        return this->extras->label;
    return this->getDefaultAccessibilityLabel();
}

void View::setAccessibilityValue(const std::string& value)
{
    Extras& extras  = this->getExtras();
    extras.valueSet = true;
    extras.value    = value;
}

std::string View::getAccessibilityValue()
{
    if (this->extras && this->extras->valueSet)
        return this->extras->value;
    return this->getDefaultAccessibilityValue();
}

void View::setAccessibilitySelected(bool selected)
{
    Extras& extras     = this->getExtras();
    extras.selectedSet = true;
    extras.selected    = selected;
}

bool View::isAccessibilitySelected()
{
    if (this->extras && this->extras->selectedSet)
        return this->extras->selected;
    return this->getDefaultAccessibilitySelected();
}

bool View::getAccessibilityPosition(int* index, int* count)
{
    switch (this->getAccessibilityRole())
    {
        case AccessibilityRole::BUTTON:
        case AccessibilityRole::CELL:
        case AccessibilityRole::TOGGLE:
        case AccessibilityRole::SLIDER:
        case AccessibilityRole::TEXT_FIELD:
        case AccessibilityRole::TAB:
            break;
        default:
            return false;
    }
    if (!this->hasParent())
        return false;

    // A recycler's cell: its row among all of its section's, shown or not.
    if (auto* cell = dynamic_cast<RecyclerCell*>(this))
    {
        View* up = this->getParent();
        for (int depth = 0; up && depth < 3; depth++, up = up->hasParent() ? up->getParent() : nullptr)
            if (auto* recycler = dynamic_cast<RecyclerFrame*>(up))
            {
                RecyclerDataSource* source = recycler->getDataSource();
                if (!source)
                    return false;
                IndexPath path = cell->getIndexPath();
                int rows       = source->numberOfRows(recycler, (int)path.section);
                if (rows < 2 || path.row < 0 || path.row >= rows)
                    return false;
                *index = path.row + 1;
                *count = rows;
                return true;
            }
    }

    int at = 0, focusable = 0;
    for (View* sibling : this->getParent()->getChildren())
    {
        if (!sibling->isFocusable())
            continue;
        focusable++;
        if (sibling == this)
            at = focusable;
    }
    if (focusable < 2 || at == 0)
        return false;
    *index = at;
    *count = focusable;
    return true;
}

// A word of the a11y catalogue, in English when the app ships none.
static std::string accessibilityWord(const std::string& key, const char* english)
{
    std::string name  = "a11y/" + key;
    std::string value = internal::getRawStr(name);
    return value == name ? english : value;
}

std::string accessibilityRoleWord(AccessibilityRole role)
{
    static const std::unordered_map<std::string, const char*> ENGLISH = {
        { "button", "button" },
        { "label", "" },
        { "header", "heading" },
        { "image", "image" },
        { "cell", "" },
        { "toggle", "switch" },
        { "slider", "slider" },
        { "text_field", "text field" },
        { "tab", "tab" },
        { "list", "list" },
        { "dialog", "dialog" },
    };
    std::string name = accessibilityRoleName(role);
    if (name.empty())
        return "";
    auto english = ENGLISH.find(name);
    return accessibilityWord("role/" + name, english != ENGLISH.end() ? english->second : name.c_str());
}

std::string accessibilityStateWord(const std::string& state)
{
    static const std::unordered_map<std::string, const char*> ENGLISH = {
        { "on", "on" },
        { "off", "off" },
        { "selected", "selected" },
        { "disabled", "disabled" },
    };
    auto english = ENGLISH.find(state);
    return accessibilityWord(state, english != ENGLISH.end() ? english->second : state.c_str());
}

std::string accessibilityPercent(float fraction)
{
    int percent        = (int)std::lround(std::clamp(fraction, 0.0f, 1.0f) * 100.0f);
    std::string format = accessibilityWord("percent", "{}%");
    try
    {
        return fmt::format(fmt::runtime(format), percent);
    }
    catch (const fmt::format_error&)
    {
        return std::to_string(percent) + "%";
    }
}

static void joinTo(std::string& to, const std::string& part)
{
    if (!part.empty())
        to += (to.empty() ? "" : ", ") + part;
}

std::string View::getAccessibilityDescription()
{
    std::string description;
    joinTo(description, this->getAccessibilityLabel());
    joinTo(description, accessibilityRoleWord(this->getAccessibilityRole()));
    joinTo(description, this->getAccessibilityValue());
    if (this->isAccessibilitySelected())
        joinTo(description, accessibilityStateWord("selected"));
    if (this->hasStyleState(StyleState::DISABLED))
        joinTo(description, accessibilityStateWord("disabled"));

    int index = 0, count = 0;
    if (this->getAccessibilityPosition(&index, &count))
    {
        std::string format = accessibilityWord("position", "{} of {}");
        try
        {
            joinTo(description, fmt::format(fmt::runtime(format), index, count));
        }
        catch (const fmt::format_error&)
        {
            joinTo(description, std::to_string(index) + "/" + std::to_string(count));
        }
    }
    return description;
}

// The texts under a box, its focusable views left out.
static std::string textUnder(Box* box)
{
    std::string text;
    for (View* child : box->getChildren())
        if (!child->isFocusable())
            joinTo(text, child->getAccessibilityText());
    return text;
}

std::string View::getAccessibilityText()
{
    if (this->visibility != Visibility::VISIBLE)
        return "";
    if (this->extras && this->extras->labelSet)
        return this->extras->label;
    Box* box = dynamic_cast<Box*>(this);
    if (!box)
        return this->getAccessibilityLabel();
    std::string own = this->getDefaultAccessibilityLabel();
    return own.empty() ? textUnder(box) : own;
}

void View::announceAccessibilityValue()
{
    if (Application::getCurrentFocus() != this)
        return;
    // A change the user just made: a value that changes on its own (a
    // clock, a download's share) is not read out at each step.
    if (getCPUTimeUsec() - Application::getLastInputTime() > 2000000)
        return;
    std::string text = this->getAccessibilityValue();
    if (this->isAccessibilitySelected())
        joinTo(text, accessibilityStateWord("selected"));
    Application::announce(text);
}

std::string Box::getDefaultAccessibilityLabel()
{
    return this->isFocusable() ? textUnder(this) : "";
}

} // namespace aurore

namespace aurore
{

// ---------------------------------------------------------------------------
// Theme colours
// ---------------------------------------------------------------------------

static bool sameColor(NVGcolor a, NVGcolor b)
{
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

bool View::setThemeColor(const std::string& property, const std::string& key)
{
    if (key.empty())
    {
        if (this->themeColors)
        {
            auto& colors = *this->themeColors;
            colors.erase(std::remove_if(colors.begin(), colors.end(),
                             [&property](const ThemeColor& color)
                             { return color.property == property; }),
                colors.end());
        }
        return true;
    }

    StyleValue current;
    if (!this->getStyleProperty(property, &current) || current.type != StyleValue::Type::COLOR)
    {
        Logger::error("theme: {} has no colour {}", this->describe(), property);
        return false;
    }
    Theme theme = Application::getTheme();
    if (!theme.hasColor(key))
    {
        Logger::error("theme: unknown colour {} ({} of {})", key, property, this->describe());
        return false;
    }

    NVGcolor color = theme[key];
    if (!this->themeColors)
        this->themeColors = std::make_unique<std::vector<ThemeColor>>();
    ThemeColor* entry = nullptr;
    for (auto& themeColor : *this->themeColors)
        if (themeColor.property == property)
            entry = &themeColor;
    if (!entry)
    {
        this->themeColors->push_back({ property, key, color });
        entry = &this->themeColors->back();
    }
    entry->key     = key;
    entry->applied = color;
    this->setStyleProperty(property, StyleValue::of(color));
    return true;
}

std::string View::getThemeColor(const std::string& property)
{
    if (this->themeColors)
        for (auto& themeColor : *this->themeColors)
            if (themeColor.property == property)
                return themeColor.key;
    return "";
}

void View::onThemeChanged()
{
    if (this->extras && !this->extras->stateStyles.empty())
        this->extras->stylesDirty = true;
    if (!this->themeColors)
        return;

    Theme theme  = Application::getTheme();
    auto& colors = *this->themeColors;
    for (auto it = colors.begin(); it != colors.end();)
    {
        StyleValue current;
        bool readable = this->getStyleProperty(it->property, &current);
        // Mid-transition, the property is on its way to the transition's end;
        // under a state style, the state's colour hides the one set before
        Extras::Styled* styled = nullptr;
        if (this->extras)
        {
            if (Extras::Running* run = this->extras->findRunning(it->property))
                if (run->progress.isRunning())
                    current = run->to;
            for (auto& entry : this->extras->styled)
                if (entry.property == it->property && entry.driven)
                    styled = &entry;
        }
        if (styled)
            current = styled->base;

        // Changed by the app since: its own colour stays
        if (!readable || !sameColor(current.color, it->applied) || !theme.hasColor(it->key))
        {
            it = colors.erase(it);
            continue;
        }

        NVGcolor color = theme[it->key];
        it->applied    = color;
        if (!sameColor(color, current.color))
        {
            if (styled)
                styled->base = StyleValue::of(color);
            else
                this->setStyleProperty(it->property, StyleValue::of(color));
        }
        ++it;
    }
}

} // namespace aurore
