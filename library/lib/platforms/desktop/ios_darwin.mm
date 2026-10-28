#import <SDL2/SDL.h>
#import <SDL2/SDL_syswm.h>
#import <UIKit/UIKit.h>
#import <aurore/core/logger.hpp>
#import <aurore/core/safe_area.hpp>
#import <aurore/platforms/desktop/desktop_platform.hpp>

namespace aurore
{

ThemeVariant ios_theme()
{
    if (UIScreen.mainScreen.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark)
        return ThemeVariant::DARK;
    else
        return ThemeVariant::LIGHT;
}

bool darwin_runloop(const std::function<bool()>& runLoopImpl)
{
    @autoreleasepool
    {
        return runLoopImpl();
    }
}

uint8_t ios_battery_status()
{
#if defined(IOS)
    UIDevice.currentDevice.batteryMonitoringEnabled = true;
    return UIDevice.currentDevice.batteryState;
#else
    return 0;
#endif
}

// The window's safe area, in points (SDL's window size is in points too):
// the notch, the rounded corners, the home indicator; on a TV, its
// overscan
Insets ios_safe_area(SDL_Window* window)
{
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (window == nullptr || !SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_UIKIT)
        return {};
    UIWindow* uiWindow = info.info.uikit.window;
    if (uiWindow == nil)
        return {};
    UIEdgeInsets insets = uiWindow.safeAreaInsets;
    return { (float)insets.top, (float)insets.right, (float)insets.bottom, (float)insets.left };
}

float ios_battery()
{
#if defined(IOS)
    return UIDevice.currentDevice.batteryLevel;
#else
    return 0;
#endif
}
};
