// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/view.hpp>

namespace aurore
{

enum ProgressSpinnerSize
{
    NORMAL,
    LARGE
};

// A progress spinner
class ProgressSpinner : public View
{
  public:
    ProgressSpinner(ProgressSpinnerSize size = ProgressSpinnerSize::NORMAL);

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void willAppear(bool resetState = false) override;
    void willDisappear(bool resetState = false) override;

    /**
     * Sets the color of the spinner's bars, in place of the theme's
     * aurore/spinner/bar_color (its alpha is the faintest bar's).
     */
    void setColor(NVGcolor color);

    // barColor, the bars' colour (the theme's until setColor)
    bool getStyleProperty(const std::string& name, StyleValue* value) override;
    bool setStyleProperty(const std::string& name, const StyleValue& value) override;

    void animate(bool animate);

    static aurore::View* create();

  private:
    NVGcolor barColor;
    bool customBarColor       = false;
    Animatable animationValue = 0.0f;
    ProgressSpinnerSize size;
    void setSize(ProgressSpinnerSize size)
    {
        this->size = size;
    }

    void restartAnimation();
};

} // namespace aurore
