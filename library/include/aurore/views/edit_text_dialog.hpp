#pragma once
#include <aurore/core/bind.hpp>
#include <aurore/core/box.hpp>
#include <aurore/views/label.hpp>

namespace aurore
{
class EditTextDialog : public Box
{
  public:
    EditTextDialog();
    ~EditTextDialog() override;
    void open();
    void setText(const std::string& value);
    void setHeaderText(const std::string& value);
    void setHintText(const std::string& value);
    void setCountText(const std::string& value);
    void setCursor(int cursor);
    bool isTranslucent() override;
    void onLayout() override;
    Event<Point>* getLayoutEvent();
    Event<>* getBackspaceEvent();
    Event<>* getCancelEvent();
    Event<>* getSubmitEvent();
    /// Y: the whole text taken out (to type another in place of the one given)
    Event<>* getClearEvent();
    Event<std::string>* getClipboardEvent();
    void updateUI();

  private:
    std::string content;
    std::string hint;
    Event<Point> layoutEvent;
    Event<> backspaceEvent, cancelEvent, summitEvent, clearEvent;
    Event<std::string> clipboardEvent;
    bool init = false;

    AURORE_BIND(aurore::Label, header, "aurore/dialog/header");
    AURORE_BIND(aurore::Label, label, "aurore/dialog/label");
    AURORE_BIND(aurore::Label, count, "aurore/dialog/count");
    AURORE_BIND(aurore::Box, container, "aurore/container");
};
}