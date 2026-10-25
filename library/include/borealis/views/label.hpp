/*
    Copyright 2019-2021 natinusala
    Copyright 2019 p-sam

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

#include <borealis/core/animation.hpp>
#include <borealis/core/timer.hpp>
#include <borealis/core/view.hpp>
#ifdef OPENCC
#define Opencc_BUILT_AS_STATIC
#include <opencc.h>
#endif

namespace brls
{

enum class HorizontalAlign
{
    LEFT,
    CENTER,
    RIGHT,
};

enum class VerticalAlign
{
    BASELINE,
    TOP,
    CENTER,
    BOTTOM,
};

enum class CursorPosition
{
    UNSET = -2,
    END   = -1,
    START = 0,
};

// Some text. The Label will automatically grow as much as possible.
// If there is enough space, the label dimensions will fit the text.
// If there is not enough horizontal space available, it will wrap and expand its height.
// If there is not enough vertical space available to wrap, the text will be truncated instead.
// The truncated text will animate if the Label or one of its parents is focused.
// Animation will be disabled if the alignment is other than LEFT.
// Warning: to wrap, the label width MUST be constrained
class Label : public View
{
  public:
    Label();
    ~Label() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onLayout() override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onParentFocusGained(View* focusedView) override;
    void onParentFocusLost(View* focusedView) override;

    /**
     * Sets the text of the label.
     */
    virtual void setText(const std::string& text);

    /**
     * Sets the alignment of the text inside
     * the view. Will not move the view, only
     * the text inside.
     *
     * Default is CENTER.
     */
    void setHorizontalAlign(HorizontalAlign align);

    /**
     * Sets the alignment of the text inside
     * the view. Will not move the view, only
     * the text inside.
     *
     * Only applies to single-line labels.
     *
     * Default is CENTER.
     */
    void setVerticalAlign(VerticalAlign align);

    void setFontSize(float value);
    // Every label drawn `scale` times its size (1 by default), but those
    // that keep theirs (setFontScaled(false)). It can change at any time:
    // the labels then need laying out again (their nodes marked dirty).
    // getFontSize() gives the size asked.
    static void setFontScale(float scale);
    static float getFontScale();
    void setFontScaled(bool scaled);
    // This label drawn at most `max` times its size, whatever the scale
    // (with a scale of 1, as asked: nothing changes).
    void setFontScaleMax(float max);
    // A single line its box is too narrow for, at a scale above 1: drawn
    // smaller until it fits, never below the size asked (then cut as
    // before), rather than wrapped. Nothing changes at a scale of 1.
    void setShrinkToFit(bool shrink);
    bool isShrinkingToFit();
    // The size it is drawn at, and the same before any shrinking to fit
    // (what the text is measured at).
    float getDrawnFontSize();
    float getFullFontSize();
    void setFontQuality(float value);
    void setLineHeight(float value);
    void setTextColor(NVGcolor color);

    int getFont();
    float getFontSize();
    float getFontQuality();
    float getLineHeight();
    NVGcolor getTextColor();

    std::string getFullText();

    static View* create();

    void setRequiredWidth(float requiredWidth);
    void setEllipsisWidth(float ellipsisWidth);

    /**
     * If an "animated" label is too large to fit its bounds, the full text will
     * be displayed in a scrolling animation instead of being truncated
     * with an ellipsis ("...").
     *
     * Unless the autoAnimate flag is set to false, a label will automatically
     * be set to "animated" if it is focused or if one of its parent is.
     */
    void setAnimated(bool animated);

    /**
     * Controls whether focus changes automatically animates the label or not.
     * Doesn't stop the ongoing animation should there be one.
     */
    void setAutoAnimate(bool autoAnimate);

    /**
     * A single-line label will not wrap and increase its
     * height, even if the remaining vertical space allows it.
     *
     * Instead, it will always be truncated (if too large) to fit the horizontal space.
     */
    void setSingleLine(bool singleLine);

    bool isSingleLine();

    /**
     * Internal flag set by the measure function.
     */
    void setIsWrapping(bool isWrapping);

    /**
     * Simplified Chinese to Traditional Chinese
     */
    static std::string STConverter(const std::string& text);

    static inline bool OPENCC_ON = true;
    static inline float FONT_SCALE = 1.0f;
    void setCursor(int cursor);

  protected:
    std::string truncatedText;
    std::string fullText;

    int font;
    float fontSize;
    bool fontScaled = true;
    float fontScaleMax = 1e6f;
    bool shrinkToFit   = false;
    float fitScale     = 1.0f;   // from onLayout, when shrinkToFit
    float fittedWidth  = 0;      // the text's width at that scale
    float textWidth();           // the text's width as drawn
    float lineHeight;
    float fontQuality;

    NVGcolor textColor;

    float requiredWidth    = 0;
    unsigned ellipsisWidth = 0;
    size_t stringLength    = 0;

    bool singleLine = false;
    bool isWrapping = false;

    bool autoAnimate  = true;
    bool animated     = false; // should it animate?
    bool animating    = false; // currently animating?
    int cursor        = -2;
    Time cursor_blink = 0;

    Timer scrollingTimer;
    Animatable scrollingAnimation;

    void stopScrollingAnimation();
    void resetScrollingAnimation();

    void startScrollTimer();
    void onScrollTimerFinished();

    HorizontalAlign horizontalAlign = HorizontalAlign::LEFT;
    VerticalAlign verticalAlign     = VerticalAlign::CENTER;

    enum NVGalign getNVGHorizontalAlign();
    enum NVGalign getNVGVerticalAlign();
};

} // namespace brls
