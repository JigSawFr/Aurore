// SPDX-FileCopyrightText: 2021 natinusala
// SPDX-License-Identifier: Apache-2.0

#include <aurore/core/bind.hpp>

namespace aurore
{

ViewNotFoundException::ViewNotFoundException(View* owner, std::string searchedId)
{
    this->errorMessage = "Cannot find view with ID \"" + searchedId + "\" in owner view " + owner->describe();
}

ViewNotFoundException::ViewNotFoundException(Activity* owner, std::string searchedId)
{
    this->errorMessage = "Cannot find view with ID \"" + searchedId + "\" in given owner activity";
}

const char* ViewNotFoundException::what() const noexcept
{
    return this->errorMessage.c_str();
}

} // namespace aurore
