/*
    Copyright 2019-2021 natinusala
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

#include <aurore/views/label.hpp>

#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/applet_frame.hpp>
#include <aurore/views/button.hpp>
#include <aurore/views/rectangle.hpp>

namespace aurore
{

// TODO: Add the blurred dialog type once the blur is finished

class DialogButton
{
  public:
    std::string label;
    VoidEvent::Callback cb;
};

// A modal dialog with zero to three buttons
// and anything as content
// Create the dialog then use open() and close()
class Dialog : public Box
{
  private:
    AURORE_BIND(Box, container, "aurore/dialog/container");
    AURORE_BIND(AppletFrame, appletFrame, "aurore/dialog/applet");

    unsigned frameX, frameY, frameWidth, frameHeight;

    std::vector<DialogButton*> buttons;
    std::function<void(void)> closeCallback = [] {};

    void rebuildButtons();
    void buttonClick(DialogButton* button);

    bool cancelable           = true;
    bool dismissOnButtonClick = true;
    Label* textLabel          = nullptr; // the text of Dialog(text)

  protected:
    AURORE_BIND(Button, button1, "aurore/dialog/button1");
    AURORE_BIND(Button, button2, "aurore/dialog/button2");
    AURORE_BIND(Button, button3, "aurore/dialog/button3");

    AURORE_BIND(Rectangle, button2separator, "aurore/dialog/button2/separator");
    AURORE_BIND(Rectangle, button3separator, "aurore/dialog/button3/separator");

    // A dialog, said with its text when the focus enters it.
    AccessibilityRole getDefaultAccessibilityRole() override { return AccessibilityRole::DIALOG; }
    std::string getDefaultAccessibilityLabel() override { return this->container->getAccessibilityText(); }

  public:
    Dialog(std::string text);
    Dialog(Box* contentView);
    ~Dialog();

    AppletFrame* getAppletFrame() override;

    /**
     * Adds a button to this dialog, with a maximum of three
     * The position depends on the add order
     *
     * Adding a button after the dialog has been opened is
     * NOT SUPPORTED
     */
    void addButton(std::string label, VoidEvent::Callback cb);

    /**
     * A cancelable dialog is closed when
     * the user presses B (defaults to true)
     *
     * A dialog without any buttons cannot
     * be cancelable
     */
    void setCancelable(bool cancelable);
    void setCloseCallback(std::function<void(void)> cb);

    /**
     * The text of a dialog made with Dialog(text), changed while it is open
     * (a progress, a countdown). Nothing for a dialog with its own view.
     */
    void setText(const std::string& text);
    std::string getText();

    /**
     * Relabels the button at `index` (0 to 2, in the order they were added).
     */
    void setButtonText(size_t index, const std::string& label);

    /**
     * When false, a button runs its callback and leaves the dialog open (it
     * closes it with close() when it is done): a dialog that shows progress,
     * a button that retries. True by default.
     */
    void setDismissOnButtonClick(bool dismiss);

    bool isCancelable() const
    {
        return cancelable;
    }

    virtual void open();
    void close(std::function<void(void)> cb = [] {});

    bool isTranslucent() override
    {
        return true;
    }
};

} // namespace aurore
