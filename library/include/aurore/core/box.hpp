// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/view.hpp>

namespace aurore
{

/// How a box spreads its children along its axis (`justifyContent` in XML).
enum class JustifyContent
{
    FLEX_START,
    CENTER,
    FLEX_END,
    SPACE_BETWEEN,
    SPACE_AROUND,
    SPACE_EVENLY,
};

/// How a box places its children across its axis (`alignItems` in XML).
enum class AlignItems
{
    AUTO,
    FLEX_START,
    CENTER,
    FLEX_END,
    STRETCH,
    BASELINE,
    SPACE_BETWEEN,
    SPACE_AROUND,
};

/// The direction a box lays its children out in (`axis` in XML).
enum class Axis
{
    ROW,
    COLUMN,
};

/// Whether a box's children go on to new lines when they do not fit (`flexWrap`).
enum class Wrap
{
    NO_WRAP,
    WRAP,
    WRAP_REVERSE,
};

/// The writing direction a box lays its children out in.
enum class Direction
{
    INHERIT,
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT,
};

/// Generic FlexBox layout
class Box : public View
{
  public:
    /// An empty box laying its children out along `flexDirection`.
    Box(Axis flexDirection);
    /// An empty row.
    Box();
    /// Deletes the children (those a ptrLock() holds go to the deletion pool).
    ~Box();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    View* getDefaultFocus() override;
    View* hitTest(Point point) override;
    View* getNextFocus(FocusDirection direction, View* currentView) override;
    void willAppear(bool resetState) override;
    void willDisappear(bool resetState) override;
    void onWindowSizeChanged() override;
    void onThemeChanged() override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onParentFocusGained(View* focusedView) override;
    void onParentFocusLost(View* focusedView) override;
    bool applyXMLAttribute(std::string name, std::string value) override;

    static View* create();

    /// The focus about to move from `from` to `newFocus`: a box may give another view
    /// instead. Asked of every parent up the tree; returns `newFocus` by default.
    virtual View* getParentNavigationDecision(View* from, View* newFocus, FocusDirection direction);

    /**
     * Adds a view to this Box.
     * Returns the position the view was added at.
     */
    virtual void addView(View* view);

    /**
     * Adds a view to this Box at the given position.
     * Returns the position the view was added at.
     */
    virtual void addView(View* view, size_t position);

    /**
     * Removes the given view from the Box. It will be freed.
     */
    virtual void removeView(View* view, bool free = true);

    /**
     * Removes all views from the Box. Them will be freed.
     */
    virtual void clearViews(bool free = true);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     *
     * Only does one layout pass instead of four when using the four methods separately.
     */
    virtual void setPadding(float padding);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     *
     * Only does one layout pass instead of four when using the four methods separately.
     */
    virtual void setPadding(float top, float right, float bottom, float left);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingTop(float top);

    float getPaddingTop();

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingRight(float right);

    float getPaddingRight();

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingBottom(float bottom);

    float getPaddingBottom();

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    virtual void setPaddingLeft(float left);

    float getPaddingLeft();

    /**
     * The edges of the box kept clear of the window's safe area (display
     * cutouts, rounded corners, system bars, overscan:
     * Application::getSafeArea): on those, the safe area is added to the
     * box's padding, so its children move in while its background still
     * reaches the window's edge. `safeArea="top|bottom"` in XML.
     *
     * AUTO (the default) is NONE, but ALL for the content view of an
     * activity; AppletFrame sets its own. A box that keeps clear of an edge
     * does it for the views in it: they take the safe area no more (a box
     * that sets NONE lets them). For a box at the window's edges: the
     * safe area is the window's, not the box's.
     *
     * A fixed height (or width) is the whole box with the safe area in it,
     * unless the box is CONTENT_BOX (setBoxSizing): it then grows by it.
     */
    virtual void setSafeArea(SafeAreaEdges edges);
    SafeAreaEdges getSafeArea() const { return this->safeArea; }

    /// The safe area this box adds to its padding now, in layout units (zero
    /// on the edges it does not keep clear of). getPaddingTop() and the
    /// others give the box's own padding, without it.
    Insets getSafeAreaPadding() const { return this->safePadding; }

    void applySafeArea(SafeAreaEdges reach) override;

    /**
     * Sets the gap between child elements inside this view.
     * This is typically used in layouts like grids or lists.
     */
    virtual void setGap(float gap);

    /**
     * Returns the current gap between child elements inside this view.
     */
    float getGap();

    /**
     * Sets the gap between two lines of children (wrapped rows, or
     * the children of a column), overriding setGap() on that axis.
     */
    void setRowGap(float gap);
    float getRowGap();

    /**
     * Sets the gap between two children along a row (or between
     * wrapped columns), overriding setGap() on that axis.
     */
    void setColumnGap(float gap);
    float getColumnGap();

    /**
     * Sets the children alignment along the Box axis.
     *
     * Default is FLEX_START.
     */
    void setJustifyContent(JustifyContent justify);

    /**
     * Sets the children alignment along the Box cross axis.
     *
     * Default is AUTO.
     */
    void setAlignItems(AlignItems alignment);

    /**
     * Sets the direction of the box, aka place the views
     * left to right or right to left (flips the children).
     *
     * Default is INHERIT.
     */
    void setDirection(Direction direction);

    /// The axis the children are laid out along.
    void setAxis(Axis axis);
    Axis getAxis() const;

    /**
     * Sets the wrapping behavior for the children inside this view.
     */
    void setWrap(Wrap wrap);

    /**
     * Returns the wrapping behavior for the children inside this view.
     */
    Wrap getWrap() const;

    /// The children, in order. The box owns them: it deletes them with itself.
    std::vector<View*>& getChildren();

    /**
     * Returns the bounds used for culling children.
     */
    virtual void getCullingBounds(float* top, float* right, float* bottom, float* left);

    /**
     * Registers an XML attribute to be forwarded to the given view. Works regardless of the target attribute type.
     * Useful to expose attributes of children views in the parent box without copy pasting them individually.
     *
     * The forwarded attribute value will override the value of the regular attribute if it already exists in the target view.
     */
    void forwardXMLAttribute(std::string attributeName, View* target);

    /**
     * Registers an XML attribute to be forwarded to the given view, while changing the target attribute name.
     * Works regardless of the target attribute type.
     * Useful to expose attributes of children views in the parent box without copy pasting them individually, but with a different name.
     *
     * The forwarded attribute value will override the value of the regular attribute if it already exists in the target view.
     */
    void forwardXMLAttribute(std::string attributeName, View* target, std::string targetAttributeName);

    /**
     * Fired when focus is gained on one of this view's children, or one of the children
     * of the children...
     *
     * directChild is guaranteed to be one of your children. It may not be the view that has been
     * focused.
     *
     * If focusedView == directChild, then the child of yours has been focused.
     * Otherwise, focusedView is a child of directChild.
     */
    virtual void onChildFocusGained(View* directChild, View* focusedView);

    /**
     * Fired when focus is lost on one of this view's children. Works similarly to
     * onChildFocusGained().
     */
    virtual void onChildFocusLost(View* directChild, View* focusedView);

    View* getView(std::string id) override;

    /// The child the focus goes back to when the box gets it again.
    void setLastFocusedView(View* view);

    /// The child that takes the focus first (negative values are ignored).
    void setDefaultFocusedIndex(int index);

    size_t getDefaultFocusedIndex() const;

    /// See setLastFocusedView().
    View* getLastFocusedView() const;

    /// Whether the focus is on one of the children, at any depth.
    virtual bool isChildFocused();

  private:
    Axis axis;

    std::vector<View*> children;

    SafeAreaEdges safeArea      = SafeAreaEdges::AUTO;
    SafeAreaEdges safeAreaReach = SafeAreaEdges::NONE; // what reached the box last
    Insets safePadding;

    size_t defaultFocusedIndex = 0;
    View* lastFocusedView      = nullptr;

    std::unordered_map<std::string, std::pair<std::string, View*>> forwardedAttributes;

  protected:
    /// A focusable box says the texts shown in it (getAccessibilityText).
    std::string getDefaultAccessibilityLabel() override;

    /// The edges the box keeps clear of when `reach` reached it
    virtual SafeAreaEdges appliedSafeArea(SafeAreaEdges reach);

    /**
     * The edges of the safe area that reach `child`, given those that
     * reached this box and those it kept clear of: by default all of them
     * when it kept clear of none, else none. A box that places its children
     * at different edges (AppletFrame: its header at the top, its footer at
     * the bottom) gives each its own.
     */
    virtual SafeAreaEdges childSafeAreaReach(View* child, SafeAreaEdges reach, SafeAreaEdges applied);

    /// Applies the safe area again to this box and the views in it, with
    /// what reached it last (its edges changed)
    void reapplySafeArea();

    /**
     * Inflates the Box with the given XML string.
     *
     * The root element MUST be a aurore::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root aurore::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLString(std::string_view xml);

    /**
     * Inflates the Box with the given XML element.
     *
     * The root element MUST be a aurore::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root aurore::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLElement(tinyxml2::XMLElement* element);

    /**
     * Inflates the Box with the given XML resource.
     *
     * The root element MUST be a aurore::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root aurore::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLRes(const std::string& res);

    /**
     * Inflates the Box with the given XML file path.
     *
     * The root element MUST be a aurore::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root aurore::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXMLFile(const std::string& path);

    /**
     * Handles a child XML element.
     *
     * By default, calls createFromXMLElement() and adds the result
     * to the children of the Box.
     */
    void handleXMLElement(tinyxml2::XMLElement* element) override;
};

/// An empty view that has auto x auto and grow=1.0 to push
/// all the next views in its box to the right (or to the bottom)
class Padding : public View
{
  public:
    Padding();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;

    static View* create();
};

} // namespace aurore
