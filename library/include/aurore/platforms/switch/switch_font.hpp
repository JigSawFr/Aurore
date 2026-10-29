// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/font.hpp>

namespace aurore
{

// Font loader using the pl system service
class SwitchFontLoader : public FontLoader
{
  public:
    void loadFonts() override;
};

} // namespace aurore
