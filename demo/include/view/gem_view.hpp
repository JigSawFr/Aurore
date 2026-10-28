/*
    Copyright 2021 XITRIX

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#pragma once

#include <borealis.hpp>

class Gem
{
  public:
    std::string id;
    std::string name;

    Gem(std::string id, std::string name)
        : id(id)
        , name(name)
    {
    }
};

class GemView : public brls::Box
{
  public:
    GemView(Gem gem);
    GemView()
        : GemView(Gem("amber", "Amber"))
    {
    }

    static brls::View* create();

  private:
    Gem gem;
    BRLS_BIND(brls::Image, image, "image");
    BRLS_BIND(brls::Label, description, "description");
};
