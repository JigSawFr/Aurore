// SPDX-FileCopyrightText: 2021 CrustySeanPro
// SPDX-License-Identifier: Apache-2.0

#include "aurore/views/widgets/account.hpp"

#include <cstring>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace aurore
{

#ifdef __SWITCH__
void setIcon(AccountUid id, Image* image)
{
    AccountProfile profile;
    AccountProfileBase profilebase;
    memset(&profilebase, 0, sizeof(profilebase));

    if (R_SUCCEEDED(accountGetPreselectedUser(&id)))
    {
        accountGetProfile(&profile, id);

        u32 iconsize = 0;
        if (R_SUCCEEDED(accountProfileGetImageSize(&profile, &iconsize)))
        {
            u8* icon     = new u8[iconsize];
            u32 fakesize = 0;

            if (R_SUCCEEDED(accountProfileLoadImage(&profile, icon, iconsize, &fakesize)))
            {
                image->setImageFromMem(icon, iconsize);
            }

            delete[] icon;
        }
        accountProfileClose(&profile);
    }
}
#endif

AccountWidget::AccountWidget()
{
#ifdef __SWITCH__
    setSize(Size(44, 44));

    Result rc = accountInitialize(AccountServiceType_Application);
    if (R_FAILED(rc))
    {
        aurore::Logger::error("accountInitialize() failed: {}\n", rc);
    }

    AccountUid userID = { 0 };
    if (R_SUCCEEDED(rc))
    {
        rc = accountGetPreselectedUser(&userID);

        if (R_FAILED(rc))
        {
            aurore::Logger::error("accountGetPreselectedUser() failed: {}\n", rc);
        }
    }

    acc = new Image();
    acc->setSize(Size(44, 44));
    acc->setScalingType(ImageScalingType::FIT);
    acc->detach();

    setIcon(userID, acc);

    addView(acc);
#else
    setVisibility(Visibility::GONE);
#endif
}

View* AccountWidget::create()
{
    return new AccountWidget();
}

} // namespace aurore
