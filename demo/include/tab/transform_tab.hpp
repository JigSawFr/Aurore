/*
    Copyright 2024 xfangfang

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

#include <aurore.hpp>

class TransformBox : public aurore::Image
{
  public:
    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style, aurore::FrameContext* ctx) override;

    static aurore::View* create();

    void setRotate(float deg);

    void setSkewX(float deg);

    void setSkewY(float deg);

    void setScaleX(float size);

    void setScaleY(float size);

    void setFontScaleX(float size);

    void setFontScaleY(float size);

  private:
    float skew_x {}, skew_y {}, rotate {}, scale_x { 1 }, scale_y { 1 };
    float font_scale_x { 1 }, font_scale_y { 1 };
};

class TransformTab : public aurore::Box
{
  public:
    TransformTab();

    static aurore::View* create();

  private:
    AURORE_BIND(TransformBox, box, "box");
    AURORE_BIND(aurore::SliderCell, transX, "transX");
    AURORE_BIND(aurore::SliderCell, transY, "transY");
    AURORE_BIND(aurore::SliderCell, scaleX, "scaleX");
    AURORE_BIND(aurore::SliderCell, scaleY, "scaleY");
    AURORE_BIND(aurore::SliderCell, skewX, "skewX");
    AURORE_BIND(aurore::SliderCell, skewY, "skewY");
    AURORE_BIND(aurore::SliderCell, rotate, "rotate");
    AURORE_BIND(aurore::SliderCell, boxWidth, "width");
    AURORE_BIND(aurore::SliderCell, boxHeight, "height");
    AURORE_BIND(aurore::SliderCell, fontScaleX, "fontScaleX");
    AURORE_BIND(aurore::SliderCell, fontScaleY, "fontScaleY");
    AURORE_BIND(aurore::Button, reset, "reset");
    AURORE_BIND(aurore::Button, play, "play");

    static void registerCell(aurore::SliderCell* cell, float init, const std::string& title, const std::function<float(float)>& cb);

    aurore::Animatable aniX, aniY, skew, skew2;
};
