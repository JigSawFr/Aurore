// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-FileCopyrightText: 2023 xfangfang
// SPDX-License-Identifier: Apache-2.0

#include <strings.h>

#include <cctype>
#include <cmath>
#include <memory>
#include <aurore/core/i18n.hpp>
#include <aurore/core/logger.hpp>
#include <aurore/core/util.hpp>
#include <aurore/platforms/desktop/desktop_platform.hpp>
#include <aurore/platforms/desktop/steam_deck.hpp>

#ifdef __SDL3__
#include <SDL3/SDL_misc.h>
#elif defined(__SDL2__)
#include <SDL2/SDL_misc.h>
#endif

#ifdef _WIN32
// The order matters: winsock2 before iphlpapi, winioctl before ntddvdeo
// clang-format off
#include <winsock2.h>
#include <iphlpapi.h>
#include <wlanapi.h>
#include <shellapi.h>
#include <winioctl.h>
#include <ntddvdeo.h>
// clang-format on
#elif IOS || TVOS
#elif __APPLE__
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <SystemConfiguration/SystemConfiguration.h>
#endif

#ifdef __WINRT__
// clang-format off
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Networking.Connectivity.h>
#include <winrt/Windows.Devices.WiFi.h>
#include <winrt/Windows.UI.ViewManagement.h>
// clang-format on
using winrt::Windows::Devices::WiFi::WiFiAdapter;
using winrt::Windows::UI::ViewManagement::UIColorType;
using winrt::Windows::UI::ViewManagement::UISettings;
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <arpa/inet.h>
#include <ifaddrs.h>
#endif

#if (defined(__APPLE__) || defined(__linux__)) && !defined(ANDROID) && !defined(IOS) && !defined(TVOS)
#include <spawn.h>
#include <sys/wait.h>

#include <cstring>
#include <thread>
#ifdef __APPLE__
#include <crt_externs.h>
#else
extern char** environ;
#endif
#endif

namespace aurore
{
#ifdef __WINRT__

const static auto timeout = std::chrono::milliseconds(500);

int winrt_wlan_quality()
{
    auto async = WiFiAdapter::FindAllAdaptersAsync();
    auto code  = async.wait_for(timeout);
    if (code == winrt::Windows::Foundation::AsyncStatus::Completed)
    {
        auto adapters = async.GetResults();
        for (auto it : adapters)
        {
            auto profileAsync = it.NetworkAdapter().GetConnectedProfileAsync();
            auto profileCode  = profileAsync.wait_for(timeout);
            if (profileCode == winrt::Windows::Foundation::AsyncStatus::Completed)
            {
                auto profile = profileAsync.GetResults();
                if (profile != nullptr && profile.IsWlanConnectionProfile())
                {
                    return int(profile.GetNetworkConnectivityLevel());
                }
            }
        }
    }
    return -1; // No WiFi Adapter found.
}
#elif defined(_WIN32)
void shell_open(const char* command)
{
    std::vector<WCHAR> wcmd(strlen(command) + 1);
    MultiByteToWideChar(CP_UTF8, 0, command, -1, wcmd.data(), wcmd.size());
    ShellExecuteW(nullptr, L"open", wcmd.data(), nullptr, nullptr, SW_SHOWNORMAL);
}

/// @return -1 no wifi interface
///          0 no wifi connected
int win32_wlan_quality()
{
    HANDLE hwlan;
    DWORD version;
    int quality = -1;
    if (!WlanOpenHandle(2, nullptr, &version, &hwlan))
    {
        PWLAN_INTERFACE_INFO_LIST ppinfo = nullptr;
        if (!WlanEnumInterfaces(hwlan, nullptr, &ppinfo))
        {
            for (DWORD i = 0; i < ppinfo->dwNumberOfItems; i++)
            {
                if (ppinfo->InterfaceInfo[i].isState != wlan_interface_state_connected)
                {
                    continue;
                }
                PWLAN_CONNECTION_ATTRIBUTES attrs = nullptr;
                DWORD attr_size                   = sizeof(attrs);
                if (!WlanQueryInterface(hwlan, &ppinfo->InterfaceInfo[i].InterfaceGuid,
                        wlan_intf_opcode_current_connection, nullptr,
                        &attr_size, (PVOID*)&attrs, nullptr))
                {
                    quality = attrs->wlanAssociationAttributes.wlanSignalQuality;
                    WlanFreeMemory(attrs);
                    break;
                }
            }
            WlanFreeMemory(ppinfo);
        }
        WlanCloseHandle(hwlan, nullptr);
    }
    return quality;
}
#elif IOS || TVOS
extern ThemeVariant ios_theme();
extern uint8_t ios_battery_status();
extern float ios_battery();
extern bool darwin_runloop(const std::function<bool()>& runLoopImpl);
#elif __APPLE__
extern int darwin_wlan_quality();
extern bool darwin_runloop(const std::function<bool()>& runLoopImpl);

/// @return low 7bit is current capacity
int darwin_get_powerstate()
{
    CFTypeRef blob = IOPSCopyPowerSourcesInfo();
    if (!blob)
        return -1;

    CFArrayRef list = IOPSCopyPowerSourcesList(blob);
    if (!blob)
    {
        CFRelease(blob);
        return -1;
    }

    int capacity  = -1;
    CFIndex total = CFArrayGetCount(list);
    for (CFIndex i = 0; i < total; i++)
    {
        CFTypeRef ps         = (CFTypeRef)CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef dict = IOPSGetPowerSourceDescription(blob, ps);
        if (!dict)
            continue;
        CFStringRef transport_type = (CFStringRef)CFDictionaryGetValue(dict, CFSTR(kIOPSTransportTypeKey));
        // Not a battery?
        if (transport_type && CFEqual(transport_type, CFSTR(kIOPSInternalType)))
        {
            CFStringRef psstat_ref    = (CFStringRef)CFDictionaryGetValue(dict, CFSTR(kIOPSPowerSourceStateKey));
            CFBooleanRef charging_ref = (CFBooleanRef)CFDictionaryGetValue(dict, CFSTR(kIOPSIsChargingKey));
            CFNumberRef capacity_ref  = (CFNumberRef)CFDictionaryGetValue(dict, CFSTR(kIOPSCurrentCapacityKey));
            if (capacity_ref != nullptr)
                CFNumberGetValue(capacity_ref, kCFNumberIntType, &capacity);
            if (charging_ref && CFBooleanGetValue(charging_ref))
                capacity |= 0x80;
            if (psstat_ref && CFEqual(psstat_ref, CFSTR(kIOPSACPowerValue)))
                capacity |= 0x80;
            break;
        }
    }
    CFRelease(list);
    CFRelease(blob);
    return capacity;
}
#elif defined(__SDL__)
#elif defined(__linux__)
// Thanks to: https://github.com/videolan/vlc/blob/master/modules/misc/inhibit/dbus.c
enum INHIBIT_TYPE
{
    FDO_SS, /**< KDE >= 4 and GNOME >= 3.10 */
    FDO_PM, /**< KDE and GNOME <= 2.26 and Xfce */
    MATE, /**< >= 1.0 */
    GNOME, /**< GNOME 2.26..3.4 */
    NONE,
};

static const char dbus_service[][40] = {
    "org.freedesktop.ScreenSaver",
    "org.freedesktop.PowerManagement",
    "org.mate.SessionManager",
    "org.gnome.SessionManager",
};

static const char dbus_interface[][40] = {
    "org.freedesktop.ScreenSaver",
    "org.freedesktop.PowerManagement.Inhibit",
    "org.mate.SessionManager",
    "org.gnome.SessionManager",
};

static const char dbus_path[][41] = {
    "/ScreenSaver",
    "/org/freedesktop/PowerManagement/Inhibit",
    "/org/mate/SessionManager",
    "/org/gnome/SessionManager",
};

static const char dbus_method_uninhibit[][10] = {
    "UnInhibit",
    "UnInhibit",
    "Uninhibit",
    "Uninhibit",
};

static const char dbus_method_inhibit[] = "Inhibit";

static INHIBIT_TYPE systemType = NONE;

static inline void probeInhibitor(DBusConnection* conn)
{
    // No session bus (a headless run, a container): libdbus asserts on a
    // null connection and aborts the program
    if (!conn)
    {
        Logger::warning("No D-Bus session: the screen may dim while the app runs");
        return;
    }

    for (int i = FDO_SS; i < NONE; ++i)
    {
        if (dbus_bus_name_has_owner(conn, dbus_service[i], NULL))
        {
            Logger::info("Found inhibitor service: {}", dbus_service[i]);
            systemType = static_cast<INHIBIT_TYPE>(i);
            return;
        }
    }
    Logger::error("Failed to find an inhibitor service");
}

static DBusConnection* connectSessionBus()
{
    DBusConnection* bus;
    DBusError err;

    dbus_error_init(&err);

    bus = dbus_bus_get_private(DBUS_BUS_SESSION, &err);

    if (!bus)
    {
        Logger::error("Could not connect to bus: {}", err.message);
        dbus_error_free(&err);
    }

    return bus;
}

void closeSessionBus(DBusConnection* bus)
{
    Logger::info("DBus closed");
    dbus_connection_close(bus);
    dbus_connection_unref(bus);
}

uint32_t dbusInhibit(DBusConnection* connection, const std::string& app, const std::string& reason)
{
    if (systemType == NONE)
    {
        Logger::error("Idle inhibitor not available");
        return 0;
    }

    DBusMessage* msg = dbus_message_new_method_call(dbus_service[systemType],
        dbus_path[systemType],
        dbus_interface[systemType],
        dbus_method_inhibit);
    if (!msg)
    {
        Logger::error("DBus cannot create new method call: {};{};{};{}", dbus_service[systemType],
            dbus_path[systemType],
            dbus_interface[systemType],
            dbus_method_inhibit);
        return 0;
    }

    const char* app_ptr    = app.c_str();
    const char* reason_ptr = reason.c_str();
    switch (systemType)
    {
        case MATE:
        case GNOME:
        {
            dbus_uint32_t xid    = 0;
            dbus_uint32_t gflags = 0xC;
            dbus_message_append_args(msg,
                DBUS_TYPE_STRING, &app_ptr,
                DBUS_TYPE_UINT32, &xid,
                DBUS_TYPE_STRING, &reason_ptr,
                DBUS_TYPE_UINT32, &gflags,
                DBUS_TYPE_INVALID);
            break;
        }
        default:
            dbus_message_append_args(msg,
                DBUS_TYPE_STRING, &app_ptr,
                DBUS_TYPE_STRING, &reason_ptr,
                DBUS_TYPE_INVALID);
            break;
    }

    DBusError dbus_error;
    dbus_error_init(&dbus_error);
    DBusMessage* dbus_reply = dbus_connection_send_with_reply_and_block(connection, msg, DBUS_TIMEOUT_USE_DEFAULT,
        &dbus_error);
    dbus_message_unref(msg);
    if (!dbus_reply)
    {
        Logger::error("DBus connection failed: {}/{}", dbus_error.name, dbus_error.message);
        dbus_error_free(&dbus_error);
        return 0;
    }

    uint32_t id               = 0;
    dbus_bool_t dbus_args_res = dbus_message_get_args(dbus_reply, &dbus_error, DBUS_TYPE_UINT32, &id,
        DBUS_TYPE_INVALID);
    dbus_message_unref(dbus_reply);
    if (!dbus_args_res)
    {
        Logger::error("DBus cannot parse replay: {}/{}", dbus_error.name, dbus_error.message);
        dbus_error_free(&dbus_error);
        return 0;
    }
    return id;
}

void dbusUnInhibit(DBusConnection* connection, uint32_t cookie)
{
    if (systemType == NONE)
    {
        Logger::error("Idle inhibitor not available");
        return;
    }

    DBusMessage* msg = dbus_message_new_method_call(dbus_service[systemType],
        dbus_path[systemType],
        dbus_interface[systemType],
        dbus_method_uninhibit[systemType]);
    if (!msg)
    {
        Logger::error("DBus cannot create new method call: {};{};{};{}", dbus_service[systemType],
            dbus_path[systemType],
            dbus_interface[systemType],
            dbus_method_uninhibit[systemType]);
        return;
    }

    dbus_message_append_args(msg,
        DBUS_TYPE_UINT32, &cookie,
        DBUS_TYPE_INVALID);

    DBusError dbus_error;
    dbus_error_init(&dbus_error);
    DBusMessage* dbus_reply = dbus_connection_send_with_reply_and_block(connection, msg, DBUS_TIMEOUT_USE_DEFAULT,
        &dbus_error);
    dbus_message_unref(msg);
    if (!dbus_reply)
    {
        Logger::error("DBus connection failed: {}/{}", dbus_error.name, dbus_error.message);
        dbus_error_free(&dbus_error);
    }
}

std::string portalInhibit(DBusConnection* connection, const std::string& reason)
{
    constexpr const char* service   = "org.freedesktop.portal.Desktop";
    constexpr const char* path      = "/org/freedesktop/portal/desktop";
    constexpr const char* interface = "org.freedesktop.portal.Inhibit";
    constexpr dbus_uint32_t idle    = 8;

    DBusMessage* msg = dbus_message_new_method_call(service, path, interface, "Inhibit");
    if (!msg)
    {
        Logger::error("Portal cannot create idle inhibitor call");
        return {};
    }

    DBusMessageIter args;
    DBusMessageIter options;
    DBusMessageIter entry;
    DBusMessageIter variant;
    const char* parentWindow = "";
    const char* reasonKey    = "reason";
    const char* reasonValue  = reason.c_str();

    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parentWindow);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &idle);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &options);
    dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &reasonKey);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &reasonValue);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(&options, &entry);
    dbus_message_iter_close_container(&args, &options);

    DBusError error;
    dbus_error_init(&error);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(
        connection, msg, DBUS_TIMEOUT_USE_DEFAULT, &error);
    dbus_message_unref(msg);
    if (!reply)
    {
        Logger::error("Portal idle inhibitor failed: {}/{}", error.name, error.message);
        dbus_error_free(&error);
        return {};
    }

    const char* requestPath = nullptr;
    if (!dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &requestPath, DBUS_TYPE_INVALID))
    {
        Logger::error("Portal cannot parse idle inhibitor reply: {}/{}", error.name, error.message);
        dbus_error_free(&error);
        dbus_message_unref(reply);
        return {};
    }

    std::string result = requestPath;
    dbus_message_unref(reply);
    Logger::info("Portal idle inhibitor enabled");
    return result;
}

void portalUnInhibit(DBusConnection* connection, const std::string& requestPath)
{
    DBusMessage* msg = dbus_message_new_method_call(
        "org.freedesktop.portal.Desktop", requestPath.c_str(), "org.freedesktop.portal.Request", "Close");
    if (!msg)
    {
        Logger::error("Portal cannot create idle uninhibitor call");
        return;
    }

    DBusError error;
    dbus_error_init(&error);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(
        connection, msg, DBUS_TIMEOUT_USE_DEFAULT, &error);
    dbus_message_unref(msg);
    if (!reply)
    {
        Logger::error("Portal idle uninhibitor failed: {}/{}", error.name, error.message);
        dbus_error_free(&error);
        return;
    }

    dbus_message_unref(reply);
    Logger::info("Portal idle inhibitor disabled");
}

static std::unique_ptr<DBusConnection, std::function<void(DBusConnection*)>> dbus_conn(connectSessionBus(),
    closeSessionBus);

// Whether the desktop prefers a dark appearance, from the XDG desktop portal's
// org.freedesktop.appearance color-scheme (1: dark, 2: light, 0: none). False
// without a session bus or a portal; at most half a second is waited.
static bool portalPrefersDark(DBusConnection* connection)
{
    if (!connection)
        return false;

    DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Settings", "Read");
    if (!msg)
        return false;

    const char* space = "org.freedesktop.appearance";
    const char* key   = "color-scheme";
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &space, DBUS_TYPE_STRING, &key, DBUS_TYPE_INVALID);

    DBusError error;
    dbus_error_init(&error);
    DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection, msg, 500, &error);
    dbus_message_unref(msg);
    if (!reply)
    {
        aurore::Logger::debug("dbus: no colour scheme from the desktop portal: {}", error.message ? error.message : "?");
        dbus_error_free(&error);
        return false;
    }

    // The value comes as a variant, in a variant with older portals
    bool dark = false;
    DBusMessageIter iter;
    if (dbus_message_iter_init(reply, &iter))
    {
        while (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_VARIANT)
        {
            DBusMessageIter inner;
            dbus_message_iter_recurse(&iter, &inner);
            iter = inner;
        }
        if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32)
        {
            dbus_uint32_t scheme = 0;
            dbus_message_iter_get_basic(&iter, &scheme);
            dark = scheme == 1;
        }
    }
    dbus_message_unref(reply);
    return dark;
}
#endif

const char* DesktopPlatform::environment(const std::string& name)
{
    return environmentVariable(name);
}

bool DesktopPlatform::localeFromEnvironmentVariable(std::string* locale)
{
    const char* langEnv = environment("LANG");
    if (!langEnv)
        return false;
    std::string lang = langEnv;
    bool name        = !lang.empty() && lang.size() <= 32;
    for (char c : lang)
        name = name && (std::isalnum((unsigned char)c) || c == '-');
    *locale = name ? lang : localeFromSystem(lang);
    return true;
}

DesktopPlatform::DesktopPlatform()
{
    // Theme
    const char* themeEnv       = environment("THEME");
    this->themeFromEnvironment = themeEnv != nullptr;
    if (themeEnv == nullptr)
    {
#if defined(IOS) || defined(TVOS)
        this->themeVariant = ios_theme();
#elif __APPLE__
        CFPropertyListRef propertyList = CFPreferencesCopyValue(
            CFSTR("AppleInterfaceStyle"),
            kCFPreferencesAnyApplication,
            kCFPreferencesCurrentUser,
            kCFPreferencesAnyHost);
        if (propertyList)
        {
            if (CFEqual(propertyList, CFSTR("Dark")))
            {
                this->themeVariant = ThemeVariant::DARK;
                aurore::Logger::info("Set app theme: Dark");
            }
            CFRelease(propertyList);
        }
#elif defined(__WINRT__)
        auto clr = UISettings().GetColorValue(UIColorType::Foreground);
        if (((5 * clr.G) + (2 * clr.R) + clr.B) > (8 * 128))
        {
            this->themeVariant = ThemeVariant::DARK;
            aurore::Logger::info("Set app theme: Dark");
        }
#elif defined(_WIN32)
        HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (hUxtheme)
        {
            auto fnSystemUseDarkMode = reinterpret_cast<BOOL(WINAPI*)()>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(138)));
            if (fnSystemUseDarkMode != nullptr && fnSystemUseDarkMode())
            {
                this->themeVariant = ThemeVariant::DARK;
                aurore::Logger::info("Set app theme: Dark");
            }
            FreeLibrary(hUxtheme);
        }
#elif defined(__linux__) && !defined(__SDL__)
        if (portalPrefersDark(dbus_conn.get()))
        {
            this->themeVariant = ThemeVariant::DARK;
            aurore::Logger::info("Set app theme: Dark");
        }
#endif
    }
    else if (!strcasecmp(themeEnv, "DARK"))
    {
        this->themeVariant = ThemeVariant::DARK;
    }
    this->systemThemeVariant = this->themeVariant;

    // Locale
    if (Platform::APP_LOCALE_DEFAULT == LOCALE_AUTO)
    {
        this->locale                = LOCALE_DEFAULT;
        this->localeFromEnvironment = localeFromEnvironmentVariable(&this->locale);
    }
    else
    {
        this->locale = Platform::APP_LOCALE_DEFAULT;
        aurore::Logger::info("Set app locale: {}", this->locale);
    }

#if defined(__WINRT__)
#elif defined(_WIN32)
    this->hLCD = ::CreateFileA("\\\\.\\LCD", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
#endif

    // Platform impls
    this->fontLoader = new DesktopFontLoader();
    this->imeManager = new DesktopImeManager();

#if defined(__linux__) && !defined(__SDL__)
    probeInhibitor(dbus_conn.get());
#endif
}

bool DesktopPlatform::canShowBatteryLevel()
{
#if defined(IOS) || defined(TVOS)
    return ios_battery_status() != 0;
#elif defined(__APPLE__)
    return darwin_get_powerstate() >= 0;
#elif defined(_WIN32)
    SYSTEM_POWER_STATUS status;
    if (!GetSystemPowerStatus(&status))
        return false;
    return !(status.BatteryFlag & BATTERY_FLAG_NO_BATTERY);
#else
    return false;
#endif
}

bool DesktopPlatform::canShowWirelessLevel()
{
#if defined(IOS) || defined(TVOS)
    return false;
#elif defined(__APPLE__)
    return true;
#elif defined(_WIN32)
    return true;
#else
    return false;
#endif
}

int DesktopPlatform::getBatteryLevel()
{
#if defined(IOS) || defined(TVOS)
    return ios_battery() * 100;
#elif defined(__APPLE__)
    return darwin_get_powerstate() & 0x7F;
#elif defined(_WIN32)
    SYSTEM_POWER_STATUS status;
    if (!GetSystemPowerStatus(&status))
        return 0;
    return status.BatteryLifePercent;
#else
    return 100;
#endif
}

bool DesktopPlatform::isBatteryCharging()
{
#if defined(IOS) || defined(TVOS)
    return ios_battery_status() == 2;
#elif defined(__APPLE__)
    return darwin_get_powerstate() & 0x80;
#elif defined(_WIN32)
    SYSTEM_POWER_STATUS status;
    if (!GetSystemPowerStatus(&status))
        return false;
    return (status.BatteryFlag & BATTERY_FLAG_CHARGING) != 0;
#else
    return false;
#endif
}

bool DesktopPlatform::hasWirelessConnection()
{
#if defined(IOS) || defined(TVOS)
    return false;
#elif defined(__APPLE__)
    return darwin_wlan_quality() > 0;
#elif defined(__WINRT__)
    return winrt_wlan_quality() > 0;
#elif defined(_WIN32)
    return win32_wlan_quality() > 0;
#else
    return true;
#endif
}

int DesktopPlatform::getWirelessLevel()
{
#if defined(IOS) || defined(TVOS)
    return 0;
#elif defined(__APPLE__)
    return darwin_wlan_quality();
#elif defined(__WINRT__)
    return winrt_wlan_quality();
#elif defined(_WIN32)
    return (win32_wlan_quality() * 4 - 1) / 100;
#elif defined(ANDROID)
    return 0;
#else
    return 0;
#endif
}

bool DesktopPlatform::hasEthernetConnection()
{
    bool has_eth = false;
#if defined(IOS) || defined(TVOS)
#elif defined(__APPLE__)
    SCDynamicStoreRef storeRef = SCDynamicStoreCreate(nullptr, CFSTR("FindCurrentInterface"), nullptr, nullptr);
    CFDictionaryRef globalRef  = (CFDictionaryRef)SCDynamicStoreCopyValue(storeRef, CFSTR("State:/Network/Global/IPv4"));
    CFTypeRef primaryIf        = nullptr;
    if (globalRef)
    {
        CFDictionaryGetValueIfPresent(globalRef, kSCDynamicStorePropNetPrimaryInterface, &primaryIf);
        if (primaryIf)
            CFRetain(primaryIf);
        CFRelease(globalRef);
    }
    CFRelease(storeRef);

    if (!primaryIf)
        return false;

    CFArrayRef iflist = SCNetworkInterfaceCopyAll();
    if (iflist)
    {
        CFIndex count = CFArrayGetCount(iflist);
        for (CFIndex i = 0; i < count; i++)
        {
            auto netif   = (SCNetworkInterfaceRef)CFArrayGetValueAtIndex(iflist, i);
            auto if_type = SCNetworkInterfaceGetInterfaceType(netif);
            auto if_name = SCNetworkInterfaceGetBSDName(netif);
            if (CFEqual(primaryIf, if_name))
            {
                has_eth = CFEqual(if_type, kSCNetworkInterfaceTypeEthernet);
            }
        }
        CFRelease(iflist);
    }
    CFRelease(primaryIf);
#elif defined(_WIN32)
    PIP_ADAPTER_ADDRESSES addrs = nullptr;
    ULONG outlen = sizeof(IP_ADAPTER_ADDRESSES), ret = 0, curindex = 0;
    HANDLE heap      = GetProcessHeap();
    SOCKADDR_IN addr = { .sin_family = AF_INET, .sin_addr = { { .S_addr = inet_addr("8.8.8.8") } } };
    GetBestInterfaceEx(reinterpret_cast<SOCKADDR*>(&addr), &curindex);

    do
    {
        if (addrs != nullptr)
            addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(HeapReAlloc(heap, HEAP_ZERO_MEMORY, addrs, outlen));
        else
            addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(HeapAlloc(heap, HEAP_ZERO_MEMORY, outlen));

        ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
            nullptr, addrs, &outlen);
    } while (ret == ERROR_BUFFER_OVERFLOW);

    if (ret == NO_ERROR)
    {
        for (auto cur = addrs; cur != nullptr; cur = cur->Next)
        {
            if (cur->OperStatus == IfOperStatusUp && cur->IfIndex == curindex)
            {
                has_eth = cur->IfType == IF_TYPE_ETHERNET_CSMACD;
                break;
            }
        }
    }
    else
    {
        Logger::warning("GetAdaptersAddresses failed {}", ret);
    }
    HeapFree(heap, 0, addrs);
#elif defined(ANDROID)
    return has_eth;
#endif
    return has_eth;
}

void DesktopPlatform::disableScreenDimming(bool disable, const std::string& reason, const std::string& app)
{
    if (this->screenDimmingDisabled == disable)
        return;
    this->screenDimmingDisabled = disable;

    if (disable)
    {
#ifdef __SDL__
#elif defined(__linux__)
        inhibitCookie = dbusInhibit(dbus_conn.get(), app, reason);
        if (inhibitCookie == 0)
            inhibitRequestPath = portalInhibit(dbus_conn.get(), reason);
#elif __APPLE__
        std::string sleepReason           = app + " " + reason;
        CFStringRef reasonForActivity     = ::CFStringCreateWithCString(kCFAllocatorDefault, sleepReason.c_str(),
                kCFStringEncodingUTF8);
        [[maybe_unused]] IOReturn success = IOPMAssertionCreateWithName(
            kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
            reasonForActivity, &assertionID);
#elif _WIN32
        SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
#endif
    }
    else
    {
#ifdef __SDL__
#elif defined(__linux__)
        if (inhibitCookie != 0)
        {
            dbusUnInhibit(dbus_conn.get(), inhibitCookie);
            inhibitCookie = 0;
        }
        if (!inhibitRequestPath.empty())
        {
            portalUnInhibit(dbus_conn.get(), inhibitRequestPath);
            inhibitRequestPath.clear();
        }
#elif __APPLE__
        IOPMAssertionRelease(assertionID);
#elif _WIN32
        SetThreadExecutionState(ES_CONTINUOUS);
#endif
    }
}

bool DesktopPlatform::runLoop(const std::function<bool()>& runLoopImpl)
{
#if __APPLE__
    return darwin_runloop(runLoopImpl);
#else
    return runLoopImpl();
#endif
}

bool DesktopPlatform::isScreenDimmingDisabled()
{
    return this->screenDimmingDisabled;
}

void DesktopPlatform::setBacklightBrightness(float brightness)
{
#if defined(__WINRT__)
    (void)brightness;
#elif defined(_WIN32)
    DISPLAY_BRIGHTNESS db = {
        .ucDisplayPolicy = DISPLAYPOLICY_BOTH,
        .ucACBrightness  = (UCHAR)std::floor(brightness * 100),
        .ucDCBrightness  = (UCHAR)std::floor(brightness * 100),
    };
    DeviceIoControl(this->hLCD, IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS,
        &db, sizeof(db), NULL, 0, NULL, NULL);
#elif defined(__linux__)
    setSteamDeckBrightness(brightness);
#else
    (void)brightness;
#endif
}

float DesktopPlatform::getBacklightBrightness()
{
#if defined(__WINRT__)
    return 0.0f;
#elif defined(_WIN32)
    DISPLAY_BRIGHTNESS db;
    DeviceIoControl(this->hLCD, IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS,
        NULL, 0, &db, sizeof(db), NULL, NULL);
    return db.ucACBrightness / 100.0f;
#elif defined(__linux__)
    return getSteamDeckBrightness();
#else
    return 0.0f;
#endif
}

bool DesktopPlatform::canSetBacklightBrightness()
{
#if defined(__WINRT__)
#elif defined(_WIN32)
    UCHAR abLevels[256];
    DWORD bytesReturned = 0;
    DeviceIoControl(
        this->hLCD,
        IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS,
        NULL,
        0,
        abLevels,
        sizeof(abLevels),
        &bytesReturned,
        NULL);

    for (DWORD i = 0; i < bytesReturned; i++)
    {
        if (abLevels[i])
            return true;
    }
#elif defined(__linux__)
    return isSteamDeckBrightnessSupported();
#endif
    return false;
}

std::string DesktopPlatform::getIpAddress()
{
    std::string ipaddr = "-";
#if defined(ANDROID)
#elif defined(IOS) || defined(TVOS)
#elif defined(__APPLE__) || defined(__linux__)
    struct ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) == 0)
    {
        for (auto addr = interfaces; addr != nullptr; addr = addr->ifa_next)
        {
            if (!addr->ifa_addr)
                continue;
            if (addr->ifa_addr->sa_family == AF_INET)
            {
                ipaddr = inet_ntoa(reinterpret_cast<struct sockaddr_in*>(addr->ifa_addr)->sin_addr);
            }
        }
    }
    freeifaddrs(interfaces);
#elif defined(_WIN32)
    PIP_ADAPTER_ADDRESSES addrs = nullptr;
    ULONG outlen = sizeof(IP_ADAPTER_ADDRESSES), ret = 0, curindex = 0;
    HANDLE heap      = GetProcessHeap();
    SOCKADDR_IN addr = { .sin_family = AF_INET, .sin_addr = { { .S_addr = inet_addr("8.8.8.8") } } };
    GetBestInterfaceEx(reinterpret_cast<SOCKADDR*>(&addr), &curindex);

    do
    {
        if (addrs != nullptr)
            addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(HeapReAlloc(heap, HEAP_ZERO_MEMORY, addrs, outlen));
        else
            addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(HeapAlloc(heap, HEAP_ZERO_MEMORY, outlen));

        ret = GetAdaptersAddresses(AF_INET,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
            nullptr, addrs, &outlen);
    } while (ret == ERROR_BUFFER_OVERFLOW);

    if (ret == NO_ERROR)
    {
        for (auto cur = addrs; cur != nullptr; cur = cur->Next)
        {
            if (cur->OperStatus == IfOperStatusUp && cur->IfIndex == curindex)
            {
                for (auto addr = cur->FirstUnicastAddress; addr != nullptr; addr = addr->Next)
                {
                    auto addrin = reinterpret_cast<PSOCKADDR_IN>(addr->Address.lpSockaddr);
                    ipaddr      = inet_ntoa(addrin->sin_addr);
                }
                break;
            }
        }
    }
    else
    {
        Logger::warning("GetAdaptersAddresses failed {}", ret);
    }
    HeapFree(heap, 0, addrs);
#endif
    return ipaddr;
}

std::string DesktopPlatform::getDnsServer()
{
    std::string dnssvr = "-";
#if defined(IOS) || defined(TVOS)
#elif defined(__APPLE__)
    SCPreferencesRef prefsDNS = SCPreferencesCreate(nullptr, CFSTR("DNSSETTING"), nullptr);
    CFArrayRef services       = SCNetworkServiceCopyAll(prefsDNS);
    char buffer[INET6_ADDRSTRLEN];
    if (services)
    {
        CFIndex count = CFArrayGetCount(services);
        for (CFIndex i = 0; i < count; i++)
        {
            const SCNetworkServiceRef service = (const SCNetworkServiceRef)CFArrayGetValueAtIndex(services, i);
            CFStringRef interfaceServiceID    = SCNetworkServiceGetServiceID(service);
            CFStringRef primaryservicepath    = CFStringCreateWithFormat(NULL, NULL, CFSTR("State:/Network/Service/%@/DNS"), interfaceServiceID);
            SCDynamicStoreRef dynRef          = SCDynamicStoreCreate(kCFAllocatorSystemDefault, CFSTR("DNSSETTING"), NULL, NULL);
            CFPropertyListRef propList        = SCDynamicStoreCopyValue(dynRef, primaryservicepath);
            if (propList)
            {
                CFArrayRef addresses = (CFArrayRef)CFDictionaryGetValue((CFDictionaryRef)propList, CFSTR("ServerAddresses"));
                long addressesCount  = CFArrayGetCount(addresses);
                for (long j = 0; j < addressesCount; j++)
                {
                    CFStringRef address = (CFStringRef)CFArrayGetValueAtIndex(addresses, j);
                    if (address)
                    {
                        const char* dnsAddress = CFStringGetCStringPtr(address, kCFStringEncodingMacRomanian);
                        if (dnsAddress)
                            dnssvr = dnsAddress;
                        else if (CFStringGetCString(address, buffer, sizeof(buffer), kCFStringEncodingMacRomanian))
                            dnssvr = buffer;
                    }
                }
                CFRelease(propList);
            }
            CFRelease(dynRef);
            CFRelease(primaryservicepath);
        }
        CFRelease(services);
    }
    CFRelease(prefsDNS);
#elif defined(_WIN32)
    PFIXED_INFO info = nullptr;
    ULONG outlen = sizeof(FIXED_INFO), ret = 0;
    HANDLE heap = GetProcessHeap();
    do
    {
        if (info != nullptr)
            info = reinterpret_cast<PFIXED_INFO>(HeapReAlloc(heap, HEAP_ZERO_MEMORY, info, outlen));
        else
            info = reinterpret_cast<PFIXED_INFO>(HeapAlloc(heap, HEAP_ZERO_MEMORY, outlen));
        ret = GetNetworkParams(info, &outlen);
    } while (ret == ERROR_BUFFER_OVERFLOW);

    if (ret == NO_ERROR)
    {
        dnssvr = info->DnsServerList.IpAddress.String;
        Logger::debug("Host Name: {} DNS Servers: {}", info->HostName, dnssvr);
    }
    else
    {
        Logger::warning("GetNetworkParams failed {}", ret);
    }
    HeapFree(heap, 0, info);
#elif defined(ANDROID)
#elif defined(__linux__)
#endif
    return dnssvr;
}

bool DesktopPlatform::isApplicationMode()
{
    return true;
}

void DesktopPlatform::exitToHomeMode(bool value)
{
    DesktopPlatform::RESTART_APP = !value;
}

void DesktopPlatform::forceEnableGamePlayRecording()
{
}

void DesktopPlatform::openBrowser(std::string url)
{
    if (!isBrowserUrl(url))
    {
        aurore::Logger::warning("open url: refused, not an http(s) link");
        return;
    }
    aurore::Logger::debug("open url: {}", url);
#ifdef __SDL__
    SDL_OpenURL(url.c_str());
#elif defined(__APPLE__) || defined(__linux__)
#ifdef __linux__
    if (isSteamDeck())
    {
        runSteamDeckCommand(steamOpenUrlCommand(url));
        return;
    }
#endif
    int pid = spawnProcess(browserCommand(url));
    if (pid > 0)
        std::thread([pid]
            { waitpid(pid, nullptr, 0); })
            .detach();
#elif defined(_WIN32) and !defined(__WINRT__)
    shell_open(url.c_str());
#endif
}

#if (defined(__APPLE__) || defined(__linux__)) && !defined(ANDROID) && !defined(IOS) && !defined(TVOS)
std::vector<std::string> DesktopPlatform::browserCommand(const std::string& url)
{
    if (!isBrowserUrl(url))
        return {};
#ifdef __APPLE__
    return { "open", url };
#else
    return { "xdg-open", url };
#endif
}

int DesktopPlatform::spawnProcess(const std::vector<std::string>& args)
{
    if (args.empty())
        return -1;
    std::vector<char*> argv;
    for (const std::string& arg : args)
        argv.push_back(const_cast<char*>(arg.c_str()));
    argv.push_back(nullptr);

#ifdef __APPLE__
    char** env = *_NSGetEnviron();
#else
    char** env = environ;
#endif
    pid_t pid = -1;
    int err   = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), env);
    if (err != 0)
    {
        aurore::Logger::warning("Cannot start {}: {}", args[0], strerror(err));
        return -1;
    }
    return pid;
}
#endif

std::string DesktopPlatform::getName()
{
    return "Desktop";
}

FontLoader* DesktopPlatform::getFontLoader()
{
    return this->fontLoader;
}

ImeManager* DesktopPlatform::getImeManager()
{
    return this->imeManager;
}

ThemeVariant DesktopPlatform::getThemeVariant()
{
    return this->themeVariant;
}

void DesktopPlatform::setThemeVariant(ThemeVariant theme)
{
    // The app's choice: the system's changes no longer replace it
    this->themeFollowsSystem = false;
    this->themeVariant       = theme;
}

void DesktopPlatform::systemThemeChanged(ThemeVariant theme)
{
    this->systemThemeVariant = theme;
    if (this->themeFollowsSystem)
        this->themeVariant = theme;
}

void DesktopPlatform::followSystemTheme()
{
    this->themeVariant = this->systemThemeVariant;
    // Under SDL3, its changes from now on (AURORE_THEME's never change)
    this->themeFollowsSystem = !this->themeFromEnvironment;
}

std::string DesktopPlatform::getLocale()
{
    return this->locale;
}

DesktopPlatform::~DesktopPlatform()
{
#if defined(__WINRT__)
#elif defined(_WIN32)
    if (this->hLCD)
        ::CloseHandle(this->hLCD);
#endif

    delete this->fontLoader;
    delete this->imeManager;
}

} // namespace aurore
