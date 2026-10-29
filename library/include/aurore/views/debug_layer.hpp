// SPDX-FileCopyrightText: 2021 XITRIX
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aurore/core/box.hpp>
#include <aurore/core/logger.hpp>

namespace aurore
{

class DebugLayer : public Box
{
  public:
    DebugLayer();
    ~DebugLayer() override;

  private:
    Event<Logger::TimePoint, LogLevel, std::string>::Subscription logSubscription;
};

} // namespace aurore
