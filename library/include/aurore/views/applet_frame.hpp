// SPDX-FileCopyrightText: 2019-2021 natinusala
// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/core/event.hpp>
#include <aurore/views/bottom_bar.hpp>
#include <aurore/views/image.hpp>
#include <aurore/views/label.hpp>

namespace aurore
{

enum class HeaderStyle
{
    REGULAR,
    POPUP
};

// A Horizon settings-like frame, with header and footer (no sidebar)
class AppletFrame : public Box
{
  public:
    AppletFrame();
    AppletFrame(View* contentView);

    ~AppletFrame();

    void handleXMLElement(tinyxml2::XMLElement* element) override;

    void pushContentView(View* view);
    void popContentView(std::function<void(void)> cb = [] {});
    void popToRootContentView(std::function<void(void)> cb = [] {});

    void setTitle(std::string title);
    void setIcon(std::string path);

    void setHeaderVisibility(Visibility visibility);
    void setFooterVisibility(Visibility visibility);

    /// With a top screen to show it (the New 3DS's, aurore/core/top_screen.hpp),
    /// the header goes there: TopScreenHeader shows the frame's title, and
    /// the screen below gets the header's room. Only a regular header that
    /// shows the title alone moves: one with views added to it (tools, a
    /// hint view) stays, as does a popup's. True by default; false keeps it
    /// on the app's screen (XML: headerOnTopScreen).
    void setHeaderOnTopScreen(bool onTopScreen);

    void setHeaderStyle(HeaderStyle style);
    void updateAppletFrameItem();

    View* getContentView()
    {
        return contentView;
    }

    Box* getHeader()
    {
        return header;
    }

    Box* getFooter()
    {
        return footer;
    }

    static View* create();

    void willAppear(bool resetState = false) override;

    /// Edge to edge: the frame keeps clear of the left and right of the safe
    /// area, its header of the top and its footer of the bottom (the content
    /// of either when hidden). Set, the frame keeps clear of what it is told
    /// instead, and its header and footer of nothing more.
    void setSafeArea(SafeAreaEdges edges) override;

    inline static bool HIDE_BOTTOM_BAR = false;

  private:
    AURORE_BIND(Box, header, "aurore/applet_frame/header");
    AURORE_BIND(BottomBar, footer, "aurore/applet_frame/footer");
    AURORE_BIND(Label, title, "aurore/applet_frame/title_label");
    AURORE_BIND(Image, icon, "aurore/applet_frame/title_icon");
    AURORE_BIND(Box, hintBox, "aurore/applet_frame/hint_box");

    HeaderStyle style = HeaderStyle::REGULAR;

    // The header's visibility as asked, and whether it is on the top screen
    // instead (applyHeaderVisibility())
    Visibility headerVisibility = Visibility::VISIBLE;
    bool headerOnTopScreen      = true;
    Event<bool>::Subscription topScreenSubscription;
    void applyHeaderVisibility();

    bool safeAreaManaged = true; // setSafeArea() not called

  protected:
    SafeAreaEdges appliedSafeArea(SafeAreaEdges reach) override;
    SafeAreaEdges childSafeAreaReach(View* child, SafeAreaEdges reach, SafeAreaEdges applied) override;

    std::vector<View*> contentViewStack;
    View* contentView = nullptr;

    /**
     * Sets the content view for that AppletFrame.
     * Will be placed between header and footer and expanded with grow factor
     * and width / height to AUTO.
     */
    void setContentView(View* view);
};

} // namespace aurore
