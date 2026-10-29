// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: GPL-3.0-or-later

// CadranDigits — a line of text in the display face of the Cadran boards
// (Sora Bold, OFL, resources/font/sora_bold_latin.ttf, loaded as
// "cadran/digits"; the default font when it is missing): the big figures,
// the card values, the chips. Aurore's Label draws with the one default font.
#pragma once

#include <string>
#include <vector>
#include <aurore.hpp>

/// A line of text in Cadran's display face (Sora): big figures, card values, chips.
class CadranDigits : public aurore::View
{
  public:
    CadranDigits();

    void setText(const std::string& text);
    const std::string& getText() const { return this->text; }
    void setFontSize(float size);
    void setTextColor(NVGcolor color);
    /// "textColor" (setThemeColor, XML textColor="@theme/…")
    bool getStyleProperty(const std::string& name, aurore::StyleValue* value) override;
    bool setStyleProperty(const std::string& name, const aurore::StyleValue& value) override;
    /// At most that wide: a longer text is drawn smaller to fit (0: never).
    void setFitWidth(float width);
    /// At most as wide as its parent is laid out: a longer text is drawn
    /// smaller to fit, the room beside it whatever the language (fitWidth
    /// then follows the parent).
    void setFitParent(bool fit);
    /// The figures at the font size, the words between them (min, h, a unit in
    /// any language) at half of it, on the same baseline: "40 min", "1 h 20"
    /// as a dial writes them. A text with no figure keeps one size.
    void setSmallUnits(bool small);
    /// Drawn Preferences › Text size's times larger, as the labels are
    /// (ui/text_size); off by default, the figures sized for their place.
    void setFollowsTextSize(bool follows);
    /// A new text rolls in from below while the old one leaves upwards (a
    /// counter's wheel), once a text was shown; off by default. Nothing with
    /// the motion off (ui/effects).
    void setRolls(bool rolls);
    /// The size taken again after the text size changed (ui/text_size).
    void remeasure() { this->measure(); }
    void onTextScaleChanged() override { this->measure(); }

    void draw(NVGcontext* vg, float x, float y, float width, float height,
        aurore::Style style, aurore::FrameContext* ctx) override;

    static aurore::View* create();

  protected:
    /// Said as a label's text (a chip, a card say it).
    aurore::AccessibilityRole getDefaultAccessibilityRole() override { return aurore::AccessibilityRole::LABEL; }
    std::string getDefaultAccessibilityLabel() override { return this->text; }

  private:
    std::string text;
    float fontSize       = 32.0f;
    float drawSize       = 32.0f; ///< fontSize, or less to fit fitWidth
    float fitWidth       = 0.0f;
    bool fitParent       = false;
    bool smallUnits      = false;
    bool followsTextSize = false;
    bool rolls           = false;
    /// The text before, leaving (rolls): until when (µs), and its size.
    std::string leaving;
    aurore::Time roll_since = 0;
    float leaving_size      = 32.0f;
    bool drawn              = false; ///< shown at least once: a change may roll
    NVGcolor color;
    int font;

    void measure(); ///< the view's size from the text
    /// A piece of the text: figures, or the words between them.
    struct Run
    {
        std::string text;
        bool figure; ///< drawn at the full size
    };
    std::vector<Run> runs(const std::string& text) const; ///< the text cut into figures and words
    float advance(NVGcontext* vg, float size) const; ///< its width at `size`
    /// `text` written at `size` within (x, y, width, height), at its middle.
    void write(NVGcontext* vg, const std::string& text, float size, float x, float y, float height) const;
};
