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

#include <aurore/core/animation.hpp>
#include <aurore/core/observable.hpp>
#include <aurore/core/timer.hpp>
#include <aurore/core/view.hpp>
#ifdef OPENCC
#define Opencc_BUILT_AS_STATIC
#include <opencc.h>
#endif

namespace aurore
{

/// Where a label's lines sit across its width.
enum class HorizontalAlign
{
    LEFT,
    CENTER,
    RIGHT,
};

/// Where a label's text sits in its height.
enum class VerticalAlign
{
    BASELINE,
    TOP,
    CENTER,
    BOTTOM,
};

/// An edit cursor's place in a label's text (Label::setCursor).
enum class CursorPosition
{
    UNSET = -2,
    END   = -1,
    START = 0,
};

/// Some text. The Label will automatically grow as much as possible.
/// If there is enough space, the label dimensions will fit the text.
/// If there is not enough horizontal space available, it will wrap and expand its height.
/// If there is not enough vertical space available to wrap, the text will be truncated instead.
/// The truncated text will animate if the Label or one of its parents is focused.
/// Animation will be disabled if the alignment is other than LEFT.
/// Warning: to wrap, the label width MUST be constrained
class Label : public View
{
  public:
    /// An empty label in the default font, size and colour.
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

    /// The font size asked, in layout units (drawn times the font scale).
    void setFontSize(float value);
    /// Every label drawn `scale` times its size (1 by default), but those
    /// that keep theirs (setFontScaled(false)). It can change at any time:
    /// the labels then need laying out again (their nodes marked dirty).
    /// getFontSize() gives the size asked. Application::setTextScale does
    /// both.
    static void setFontScale(float scale);
    /// See setFontScale().
    static float getFontScale();
    /// false: this label keeps its size whatever the font scale.
    void setFontScaled(bool scaled);
    /// This label drawn at most `max` times its size, whatever the scale
    /// (with a scale of 1, as asked: nothing changes).
    void setFontScaleMax(float max);
    /// A single line its box is too narrow for, at a scale above 1: drawn
    /// smaller until it fits, never below the size asked (then cut as
    /// before), rather than wrapped. Nothing changes at a scale of 1.
    void setShrinkToFit(bool shrink);
    /// See setShrinkToFit().
    bool isShrinkingToFit();
    /// With shrinkToFit: drawn as small as `size`, under the size asked and at
    /// any scale, before being cut (0, the default: the size asked).
    void setShrinkToFitMin(float size);
    /// The size it is drawn at, and the same before any shrinking to fit
    /// (what the text is measured at).
    float getDrawnFontSize();
    float getFullFontSize();
    /// nanovg's quality for the glyphs (1 by default).
    void setFontQuality(float value);
    /// The height of a line, as a multiple of the font size.
    void setLineHeight(float value);
    /// The text's colour (setThemeColor follows a theme key instead).
    void setTextColor(NVGcolor color);

    /// The font this label is drawn with (the app's default one unless set):
    /// by its id, or by the name it was loaded under (Application::loadFont*).
    /// A name not loaded leaves the font as it was. XML: fontFamily="name".
    void setFont(int font);
    bool setFontFamily(const std::string& name);

    /// The font id the label draws with.
    int getFont();
    /// The font size asked, before the font scale.
    float getFontSize();
    float getFontQuality();
    float getLineHeight();
    NVGcolor getTextColor();

    /// The whole text, as set (not cut to fit).
    std::string getFullText();

    bool getStyleProperty(const std::string& name, StyleValue* value) override;
    bool setStyleProperty(const std::string& name, const StyleValue& value) override;
    bool setBoundText(const std::string& text) override;
    void onTextScaleChanged() override;

    /// Its own Observable: the label shows its value, now and at each change,
    /// as long as it lives.
    void bindText(Observable<std::string>& text);
    /// The text as laid out does not fit: it is drawn cut, with an ellipsis.
    bool isTruncated();

    /// Makes a label for XML (`<aurore:Label>`).
    static View* create();

    /// Set by the measure function: the width the text needs on one line.
    void setRequiredWidth(float requiredWidth);
    /// Set by the measure function: the width of the ellipsis.
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

    /// See setSingleLine().
    bool isSingleLine();

    /**
     * Internal flag set by the measure function.
     */
    void setIsWrapping(bool isWrapping);

    /**
     * Simplified Chinese to Traditional Chinese
     */
    static std::string STConverter(const std::string& text);

    static inline bool OPENCC_ON   = true; ///< STConverter() applied to Traditional Chinese text
    static inline float FONT_SCALE = 1.0f; ///< The font scale (setFontScale())
    /// Draws a blinking edit cursor before the byte `cursor` (CursorPosition::END: at the end; UNSET: none).
    void setCursor(int cursor);

  protected:
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::LABEL; }
    std::string getDefaultAccessibilityLabel() override { return this->getFullText(); }

    std::string truncatedText;
    std::string fullText;

    int font;
    float fontSize;
    bool fontScaled    = true;
    float fontScaleMax = 1e6f;
    bool shrinkToFit   = false;
    float shrinkMin    = 0; ///< setShrinkToFitMin
    float fitScale     = 1.0f; ///< from onLayout, when shrinkToFit
    float fittedWidth  = 0; ///< the text's width at that scale
    float textWidth(); ///< the text's width as drawn
    float lineHeight;
    float fontQuality;

    NVGcolor textColor;

    float requiredWidth    = 0;
    unsigned ellipsisWidth = 0;
    size_t stringLength    = 0;

    bool singleLine = false;
    bool isWrapping = false;

    bool autoAnimate  = true;
    bool animated     = false; ///< should it animate?
    bool animating    = false; ///< currently animating?
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

} // namespace aurore
