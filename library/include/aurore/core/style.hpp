/*
    Copyright 2019-2020 natinusala
    Copyright 2019 WerWolv
    Copyright 2019 p-sam

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

#include <initializer_list>
#include <string>
#include <unordered_map>

namespace aurore
{

/// The metrics (sizes, margins, durations) by name, "aurore/sidebar/width"…: the ones
/// the code adds, and over them a theme file's `[metrics]`.
class StyleValues
{
  public:
    /// The base metrics.
    StyleValues(std::initializer_list<std::pair<std::string, float>> list);

    /// A metric of the base: what it is without a theme file.
    void addMetric(const std::string&, float value);
    /// A metric; an unknown name is logged as an error and gives 0.
    float getMetric(const std::string& name);
    /// Whether the metric exists.
    bool hasMetric(const std::string& name) const;

    /// The theme file's metrics, over the base (empty: the base alone).
    void setOverlay(const std::unordered_map<std::string, float>& overlay);

  private:
    std::unordered_map<std::string, float> base;
    std::unordered_map<std::string, float> overlay;
    std::unordered_map<std::string, float> values; ///< base, then overlay
};

/// Simple wrapper around StyleValues for the array operator
class Style
{
  public:
    /// A handle on `values` (not owned).
    Style(StyleValues* values);
    /// The metric `name` (getMetric()).
    float operator[](const std::string& name);

    /// Adds or changes a metric of the base.
    void addMetric(const std::string& name, float value);
    /// A metric; an unknown name is logged as an error and gives 0.
    float getMetric(const std::string& name);
    /// Whether the metric exists.
    bool hasMetric(const std::string& name);
    /// The metrics behind the handle.
    StyleValues* getValues() { return this->values; }

  private:
    StyleValues* values;
};

/// The app's metrics: what XML reads with `@style/…`.
Style getStyle();

} // namespace aurore
