# Keyboard and mouse

On the desktop, the web and with a keyboard plugged into a console, the keyboard stands in for the controller, and the mouse points and clicks as a finger does.

## The keys

| Controller | Keys |
|---|---|
| A | Enter, keypad Enter |
| B | Escape, Backspace |
| D-pad | the arrows |
| X | X |
| Y | Y |
| L, R | L, R |
| ZL, ZR | Shift+L, Shift+R |
| + | P, + (Shift+=), keypad + |
| − | M, -, keypad − |

These are the keys of PlayGuard's web simulator too. A key with Ctrl, Alt or the system key held is not one of them (Ctrl+X stays a shortcut), and none applies while a text is being typed. The keys are named after their place on a US keyboard, as GLFW and SDL name them: on a French keyboard, M is the key right of L.

Enter and Escape stay A and B when `Application::setSwapInputKeys(true)` swaps the controller's buttons; so do the keys above.

An app changes them before or after `Application::init()`:

```cpp
aurore::Application::mapKey(aurore::AURORE_KBD_KEY_Q, aurore::BUTTON_LB);   // Q as L as well
aurore::Application::unmapKey(aurore::AURORE_KBD_KEY_M);                  // M types nothing
aurore::Application::mapKey({ aurore::AURORE_KBD_KEY_F, aurore::AURORE_KBD_MODIFIER_CTRL }, aurore::BUTTON_Y);
aurore::Application::clearKeyMap();                                   // Enter, Escape and the arrows only
```

A key a view registers an action for (`registerAction(AuroreKeyCombination(...), ...)`) keeps that action only: an app that already gave X its own meaning on the desktop is not pressed twice.

## The mouse

A left click is a touch; the right button is B, the middle one X. The buttons are read from their events: a quick click or a touchpad tap, let go before the next frame, still counts.

## Where the focus is

The focus waiting on a scrolling list itself, between two lines out of sight or through a long text, has no highlight: the list's scroll bar then shows it, wider and in the focus colour, until the focus moves to a line again.

A horizontal list (`aurore:HScrollingFrame`) scrolled to its start lets Left out to the views around it, the rail first; to its end, Right.
