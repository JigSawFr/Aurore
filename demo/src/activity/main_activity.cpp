// SPDX-FileCopyrightText: 2020-2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include "activity/main_activity.hpp"

MainActivity::MainActivity()
{
    auto* input             = aurore::Application::getPlatform()->getInputManager();
    this->enterSubscription = input->getMouseCusorEntered()->subscribe([](bool enterd)
        { aurore::Logger::debug("Mouse Cusor {}", enterd ? "enterd" : "leave"); });
}

MainActivity::~MainActivity()
{
    auto* input = aurore::Application::getPlatform()->getInputManager();
    input->getMouseCusorEntered()->unsubscribe(this->enterSubscription);
}