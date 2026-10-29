// SPDX-FileCopyrightText: 2019 natinusala
// SPDX-FileCopyrightText: 2019 WerWolv
// SPDX-FileCopyrightText: 2019 p-sam
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <switch.h>
#include <unistd.h>

static int nxlink_sock = -1;

// BSD sessions (sockets open at once): an app that uses more than these at
// a time blocks in lwIP. Set with -DAURORE_BSD_SESSIONS=… (as an application
// or system application) and -DAURORE_BSD_SESSIONS_APPLET=… (as an applet,
// where memory is short).
#ifndef AURORE_BSD_SESSIONS
#define AURORE_BSD_SESSIONS 12
#endif
#ifndef AURORE_BSD_SESSIONS_APPLET
#define AURORE_BSD_SESSIONS_APPLET 2
#endif

void userAppInit()
{
    printf("userAppInit\n");
    appletLockExit();

    // Init network
    SocketInitConfig cfg = *(socketGetDefaultInitConfig());
    AppletType at        = appletGetAppletType();
    if (at == AppletType_Application || at == AppletType_SystemApplication)
    {
        cfg.num_bsd_sessions = AURORE_BSD_SESSIONS; // libnx's default is 3
        cfg.sb_efficiency    = 8; // default is 4
        socketInitialize(&cfg);
    }
    else
    {
        cfg.num_bsd_sessions = AURORE_BSD_SESSIONS_APPLET;
        cfg.sb_efficiency    = 1;
        socketInitialize(&cfg);
    }

#ifdef DEBUG
    nxlink_sock = nxlinkStdio();
#endif

    romfsInit();
    plInitialize(PlServiceType_User);
    setsysInitialize();
    setInitialize();
    psmInitialize();
    nifmInitialize(NifmServiceType_User);
    lblInitialize();
}

void userAppExit()
{
    printf("userAppExit\n");

    // backlight
    lblExit();
    // network state
    nifmExit();
    // power state
    psmExit();
    // system language...
    setExit();
    // system theme, system version...
    setsysExit();
    // system font
    plExit();

    romfsExit();

    if (nxlink_sock != -1)
        close(nxlink_sock);

    socketExit();

    appletUnlockExit();
}
